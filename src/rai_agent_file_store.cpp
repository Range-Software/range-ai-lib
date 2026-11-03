#include "rai_agent_file_store.h"

#include <QCryptographicHash>
#include <QFile>
#include <QHttpHeaders>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUrlQuery>
#include <QUuid>

#include <rbl_error.h>
#include <rbl_logger.h>

#include <rcl_http_client.h>
#include <rcl_http_client_settings.h>
#include <rcl_http_message.h>
#include <rcl_http_proxy_settings.h>

const qint64 RAgentFileStore::defaultMaxStorageSize = 100LL * 1024 * 1024 * 1024;
const qint64 RAgentFileStore::maxRemoteFileSize = 500LL * 1024 * 1024;
const qint64 RAgentFileStore::remoteFilesCacheTtlSecs = 300;

RAgentFileStore::RAgentFileStore(const RAgentSettings &settings)
    : settings{settings}
    , maxStorageSize{RAgentFileStore::defaultMaxStorageSize}
    , indexLoaded{false}
{
}

void RAgentFileStore::setSettings(const RAgentSettings &settings)
{
    QMutexLocker locker(&this->mutex);
    this->settings = settings;
    this->invalidateRemoteFilesCache();
}

void RAgentFileStore::setMaxStorageSize(qint64 maxStorageSize)
{
    QMutexLocker locker(&this->mutex);
    this->maxStorageSize = (maxStorageSize > 0) ? maxStorageSize : RAgentFileStore::defaultMaxStorageSize;
}

void RAgentFileStore::setIndexFileName(const QString &indexFileName)
{
    QMutexLocker locker(&this->mutex);
    this->indexFileName = indexFileName;
    this->indexLoaded = false;
}

QString RAgentFileStore::ensureFileUploaded(const QString &key, const QString &fileName, const QByteArray &content, const QString &mimeType)
{
    QMutexLocker locker(&this->mutex);
    try
    {
        return this->ensureFileUploadedLocked(key,fileName,content,mimeType);
    }
    catch (...)
    {
        // The remote state is unknown after a failed operation.
        this->invalidateRemoteFilesCache();
        throw;
    }
}

void RAgentFileStore::invalidateRemoteFileListing()
{
    // Must not wait for the mutex, which is held during network transfers.
    this->remoteFilesListingInvalidated = true;
}

QList<RAgentFileStore::RemoteFile> RAgentFileStore::cachedRemoteFiles()
{
    const QDateTime now = QDateTime::currentDateTimeUtc();
    if (this->remoteFilesListingInvalidated.exchange(false) ||
        !this->remoteFilesListedAt.isValid() || this->remoteFilesListedAt.secsTo(now) >= RAgentFileStore::remoteFilesCacheTtlSecs)
    {
        this->remoteFilesCache = this->listRemoteFiles();
        this->remoteFilesListedAt = now;
    }
    return this->remoteFilesCache;
}

void RAgentFileStore::updateRemoteFilesCache(const QList<RemoteFile> &remoteFiles)
{
    if (this->remoteFilesListedAt.isValid())
    {
        this->remoteFilesCache = remoteFiles;
    }
}

void RAgentFileStore::invalidateRemoteFilesCache()
{
    this->remoteFilesCache.clear();
    this->remoteFilesListedAt = QDateTime();
}

QString RAgentFileStore::ensureFileUploadedLocked(const QString &key, const QString &fileName, const QByteArray &content, const QString &mimeType)
{
    if (this->settings.getType() != RAgentSettings::Type::Anthropic)
    {
        throw RError(RError::Application,R_ERROR_REF,"Remote file store is only available for the Anthropic agent type.");
    }

    if (content.size() > RAgentFileStore::maxRemoteFileSize)
    {
        throw RError(RError::InvalidInput,R_ERROR_REF,
                     "File \"%s\" exceeds the remote file store per-file limit (size: %lld bytes, max: %lld bytes).",
                     fileName.toUtf8().constData(),qint64(content.size()),RAgentFileStore::maxRemoteFileSize);
    }
    if (content.size() > this->maxStorageSize)
    {
        throw RError(RError::InvalidInput,R_ERROR_REF,
                     "File \"%s\" exceeds the remote file store capacity (size: %lld bytes, capacity: %lld bytes).",
                     fileName.toUtf8().constData(),qint64(content.size()),this->maxStorageSize);
    }

    this->loadIndex();

    const QString sha256 = QString::fromLatin1(QCryptographicHash::hash(content,QCryptographicHash::Sha256).toHex());

    // Re-use a recent listing: a query on an already uploaded file then needs
    // no request to the file store at all.
    QList<RemoteFile> remoteFiles = this->cachedRemoteFiles();

    auto remoteFileExists = [&remoteFiles](const QString &remoteFileId)
    {
        for (const RemoteFile &remoteFile : remoteFiles)
        {
            if (remoteFile.id == remoteFileId)
            {
                return true;
            }
        }
        return false;
    };

    if (this->index.contains(key))
    {
        IndexEntry &entry = this->index[key];
        if (entry.sha256 == sha256 && remoteFileExists(entry.remoteFileId))
        {
            // Same content already uploaded: just refresh the access time.
            entry.lastAccess = QDateTime::currentDateTimeUtc();
            this->saveIndex();
            RLogger::info("[RAgentFileStore] Reusing remote file id = \"%s\" for key = \"%s\".\n",
                          entry.remoteFileId.toUtf8().constData(),
                          key.toUtf8().constData());
            return entry.remoteFileId;
        }

        // Content changed or the remote file disappeared: replace it.
        if (remoteFileExists(entry.remoteFileId))
        {
            RLogger::info("[RAgentFileStore] Content of key = \"%s\" changed, deleting remote file id = \"%s\".\n",
                          key.toUtf8().constData(),
                          entry.remoteFileId.toUtf8().constData());
            this->deleteRemoteFile(entry.remoteFileId);
            for (qsizetype i = 0; i < remoteFiles.size(); ++i)
            {
                if (remoteFiles[i].id == entry.remoteFileId)
                {
                    remoteFiles.removeAt(i);
                    break;
                }
            }
        }
        this->index.remove(key);
    }

    this->makeRoom(remoteFiles,content.size());

    const QString remoteFileId = this->uploadFile(fileName,content,mimeType);

    RemoteFile uploadedFile;
    uploadedFile.id = remoteFileId;
    uploadedFile.sizeBytes = content.size();
    uploadedFile.createdAt = QDateTime::currentDateTimeUtc();
    remoteFiles.append(uploadedFile);
    this->updateRemoteFilesCache(remoteFiles);

    IndexEntry entry;
    entry.remoteFileId = remoteFileId;
    entry.sha256 = sha256;
    entry.size = content.size();
    entry.lastAccess = QDateTime::currentDateTimeUtc();
    this->index.insert(key,entry);
    this->saveIndex();

    RLogger::info("[RAgentFileStore] Uploaded key = \"%s\" as remote file id = \"%s\" (%lld bytes).\n",
                  key.toUtf8().constData(),
                  remoteFileId.toUtf8().constData(),
                  qint64(content.size()));

    return remoteFileId;
}

void RAgentFileStore::loadIndex()
{
    if (this->indexLoaded)
    {
        return;
    }
    this->indexLoaded = true;
    this->index.clear();

    if (this->indexFileName.isEmpty() || !QFile::exists(this->indexFileName))
    {
        return;
    }

    QFile inFile(this->indexFileName);
    if (!inFile.open(QIODevice::ReadOnly))
    {
        RLogger::warning("[RAgentFileStore] Failed to open index file \"%s\" for reading. %s.\n",
                         inFile.fileName().toUtf8().constData(),
                         inFile.errorString().toUtf8().constData());
        return;
    }

    const QJsonObject json = QJsonDocument::fromJson(inFile.readAll()).object();
    inFile.close();

    const QJsonArray entries = json[QStringLiteral("entries")].toArray();
    for (const QJsonValue &value : entries)
    {
        const QJsonObject entryObject = value.toObject();
        const QString key = entryObject[QStringLiteral("key")].toString();
        if (!key.isEmpty())
        {
            this->index.insert(key,RAgentFileStore::indexEntryFromJson(entryObject));
        }
    }
}

void RAgentFileStore::saveIndex() const
{
    if (this->indexFileName.isEmpty())
    {
        return;
    }

    QJsonArray entries;
    for (auto iter = this->index.cbegin(); iter != this->index.cend(); ++iter)
    {
        QJsonObject entryObject = RAgentFileStore::indexEntryToJson(iter.value());
        entryObject[QStringLiteral("key")] = iter.key();
        entries.append(entryObject);
    }
    QJsonObject json;
    json[QStringLiteral("entries")] = entries;

    QFile outFile(this->indexFileName);
    if (!outFile.open(QIODevice::WriteOnly))
    {
        RLogger::warning("[RAgentFileStore] Failed to open index file \"%s\" for writing. %s.\n",
                         outFile.fileName().toUtf8().constData(),
                         outFile.errorString().toUtf8().constData());
        return;
    }
    outFile.write(QJsonDocument(json).toJson());
    outFile.close();
}

QString RAgentFileStore::filesApiUrl() const
{
    QString apiUrl = this->settings.getApiUrl();
    if (apiUrl.isEmpty())
    {
        return QStringLiteral("https://api.anthropic.com/v1/files");
    }
    // The configured URL points at the messages endpoint; derive the files
    // endpoint from the same API root.
    const qsizetype position = apiUrl.lastIndexOf(QStringLiteral("/v1/"));
    if (position >= 0)
    {
        return apiUrl.left(position) + QStringLiteral("/v1/files");
    }
    while (apiUrl.endsWith(QChar('/')))
    {
        apiUrl.chop(1);
    }
    return apiUrl + QStringLiteral("/v1/files");
}

QByteArray RAgentFileStore::sendRequest(const RHttpMessage &request) const
{
    RHttpClientSettings httpSettings;
    httpSettings.setUrl(this->filesApiUrl());
    httpSettings.setTimeout(static_cast<uint>(this->settings.getTimeout()));

    if (!this->settings.getProxyHost().isEmpty())
    {
        RHttpProxySettings proxy;
        proxy.setType(RHttpProxySettings::ManualProxy);
        proxy.setHost(this->settings.getProxyHost());
        proxy.setPort(static_cast<quint16>(this->settings.getProxyPort()));
        proxy.setUser(this->settings.getProxyUser());
        proxy.setPassword(this->settings.getProxyPassword());
        httpSettings.setProxySettings(proxy);
    }

    RHttpClient httpClient(RHttpClient::Type::Public,httpSettings);

    RHttpMessage reply;
    httpClient.sendRequest(request,reply);

    if (reply.getErrorType() != RError::None)
    {
        QString errorString = QString::fromUtf8(reply.getBody());
        if (errorString.isEmpty())
        {
            errorString = QStringLiteral("Unknown error.");
        }
        throw RError(RError::Connection,R_ERROR_REF,
                     "Remote file store request failed. %s",
                     errorString.toUtf8().constData());
    }

    return reply.getBody();
}

static QHttpHeaders buildFileStoreHeaders(const RAgentSettings &settings, const QByteArray &contentType)
{
    QHttpHeaders headers;
    if (!contentType.isEmpty())
    {
        headers.append(QHttpHeaders::WellKnownHeader::ContentType,contentType);
    }
    const bool isNativeAnthropicUrl = settings.getApiUrl().isEmpty() ||
                                      settings.getApiUrl().contains(QStringLiteral("anthropic.com"));
    if (isNativeAnthropicUrl)
    {
        headers.append("x-api-key",settings.getApiKey().toUtf8());
        headers.append("anthropic-version",QByteArrayLiteral("2023-06-01"));
    }
    else
    {
        headers.append("Authorization",QByteArrayLiteral("Bearer ") + settings.getApiKey().toUtf8());
    }
    headers.append("anthropic-beta",QByteArrayLiteral("files-api-2025-04-14"));
    return headers;
}

QList<RAgentFileStore::RemoteFile> RAgentFileStore::listRemoteFiles() const
{
    QList<RemoteFile> remoteFiles;

    QString afterId;
    bool hasMore = true;
    while (hasMore)
    {
        QUrlQuery urlQuery;
        urlQuery.addQueryItem(QStringLiteral("limit"),QStringLiteral("1000"));
        if (!afterId.isEmpty())
        {
            urlQuery.addQueryItem(QStringLiteral("after_id"),afterId);
        }

        RHttpMessage request;
        request.setMethod(QHttpServerRequest::Method::Get);
        request.setUrlQuery(urlQuery);
        request.setRequestHeaders(buildFileStoreHeaders(this->settings,QByteArray()));

        const QJsonObject responseObject = QJsonDocument::fromJson(this->sendRequest(request)).object();
        const QJsonArray data = responseObject[QStringLiteral("data")].toArray();
        for (const QJsonValue &value : data)
        {
            const QJsonObject fileObject = value.toObject();
            RemoteFile remoteFile;
            remoteFile.id = fileObject[QStringLiteral("id")].toString();
            remoteFile.sizeBytes = fileObject[QStringLiteral("size_bytes")].toVariant().toLongLong();
            remoteFile.createdAt = QDateTime::fromString(fileObject[QStringLiteral("created_at")].toString(),Qt::ISODateWithMs);
            remoteFiles.append(remoteFile);
        }

        hasMore = responseObject[QStringLiteral("has_more")].toBool() && !data.isEmpty();
        afterId = responseObject[QStringLiteral("last_id")].toString();
        if (afterId.isEmpty())
        {
            hasMore = false;
        }
    }

    return remoteFiles;
}

void RAgentFileStore::deleteRemoteFile(const QString &remoteFileId) const
{
    RHttpMessage request;
    request.setMethod(QHttpServerRequest::Method::Delete);
    request.setTo(remoteFileId);
    request.setRequestHeaders(buildFileStoreHeaders(this->settings,QByteArray()));

    this->sendRequest(request);
}

QString RAgentFileStore::uploadFile(const QString &fileName, const QByteArray &content, const QString &mimeType) const
{
    const QByteArray boundary = QByteArrayLiteral("RangeCloudFileBoundary") +
                                QUuid::createUuid().toString(QUuid::Id128).toUtf8();

    QString safeFileName = fileName;
    safeFileName.replace(QChar('"'),QChar('_')).replace(QChar('\\'),QChar('_'));

    QByteArray body;
    body += "--" + boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"file\"; filename=\"" + safeFileName.toUtf8() + "\"\r\n";
    body += "Content-Type: " + mimeType.toUtf8() + "\r\n";
    body += "\r\n";
    body += content;
    body += "\r\n--" + boundary + "--\r\n";

    RHttpMessage request;
    request.setMethod(QHttpServerRequest::Method::Post);
    request.setBody(body);
    request.setRequestHeaders(buildFileStoreHeaders(this->settings,QByteArrayLiteral("multipart/form-data; boundary=") + boundary));

    const QJsonObject responseObject = QJsonDocument::fromJson(this->sendRequest(request)).object();
    const QString remoteFileId = responseObject[QStringLiteral("id")].toString();
    if (remoteFileId.isEmpty())
    {
        throw RError(RError::Application,R_ERROR_REF,
                     "Remote file store upload response does not contain a file id. %s",
                     QJsonDocument(responseObject).toJson(QJsonDocument::Compact).constData());
    }
    return remoteFileId;
}

void RAgentFileStore::makeRoom(QList<RemoteFile> &remoteFiles, qint64 requiredSize)
{
    qint64 usedSize = 0;
    for (const RemoteFile &remoteFile : std::as_const(remoteFiles))
    {
        usedSize += remoteFile.sizeBytes;
    }

    while (usedSize + requiredSize > this->maxStorageSize && !remoteFiles.isEmpty())
    {
        // Evict the file with the oldest access time. Files uploaded outside
        // of this index fall back to their remote creation time.
        qsizetype oldestPosition = 0;
        QDateTime oldestAccess;
        for (qsizetype i = 0; i < remoteFiles.size(); ++i)
        {
            const QString key = this->keyForRemoteFileId(remoteFiles[i].id);
            QDateTime access = key.isEmpty() ? remoteFiles[i].createdAt : this->index[key].lastAccess;
            if (!access.isValid())
            {
                access = QDateTime::fromSecsSinceEpoch(0);
            }
            if (!oldestAccess.isValid() || access < oldestAccess)
            {
                oldestAccess = access;
                oldestPosition = i;
            }
        }

        const RemoteFile evicted = remoteFiles.takeAt(oldestPosition);
        RLogger::info("[RAgentFileStore] Evicting remote file id = \"%s\" (%lld bytes, last access: %s) to make room.\n",
                      evicted.id.toUtf8().constData(),
                      evicted.sizeBytes,
                      oldestAccess.toString(Qt::ISODate).toUtf8().constData());
        this->deleteRemoteFile(evicted.id);

        const QString key = this->keyForRemoteFileId(evicted.id);
        if (!key.isEmpty())
        {
            this->index.remove(key);
        }
        usedSize -= evicted.sizeBytes;
    }

    if (usedSize + requiredSize > this->maxStorageSize)
    {
        throw RError(RError::Application,R_ERROR_REF,
                     "Remote file store does not have enough space (required: %lld bytes, used: %lld bytes, capacity: %lld bytes).",
                     requiredSize,usedSize,this->maxStorageSize);
    }
}

QString RAgentFileStore::keyForRemoteFileId(const QString &remoteFileId) const
{
    for (auto iter = this->index.cbegin(); iter != this->index.cend(); ++iter)
    {
        if (iter.value().remoteFileId == remoteFileId)
        {
            return iter.key();
        }
    }
    return QString();
}

RAgentFileStore::IndexEntry RAgentFileStore::indexEntryFromJson(const QJsonObject &json)
{
    IndexEntry entry;
    entry.remoteFileId = json[QStringLiteral("remoteFileId")].toString();
    entry.sha256 = json[QStringLiteral("sha256")].toString();
    entry.size = json[QStringLiteral("size")].toVariant().toLongLong();
    entry.lastAccess = QDateTime::fromString(json[QStringLiteral("lastAccess")].toString(),Qt::ISODateWithMs);
    return entry;
}

QJsonObject RAgentFileStore::indexEntryToJson(const IndexEntry &entry)
{
    QJsonObject json;
    json[QStringLiteral("remoteFileId")] = entry.remoteFileId;
    json[QStringLiteral("sha256")] = entry.sha256;
    json[QStringLiteral("size")] = QString::number(entry.size);
    json[QStringLiteral("lastAccess")] = entry.lastAccess.toString(Qt::ISODateWithMs);
    return json;
}
