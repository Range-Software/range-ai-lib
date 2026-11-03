#ifndef RAI_AGENT_FILE_STORE_H
#define RAI_AGENT_FILE_STORE_H

#include <QByteArray>
#include <QDateTime>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QMutex>

#include <atomic>
#include <QString>

#include "rai_agent_settings.h"

class RHttpMessage;

//! Client for the Anthropic Files API (https://api.anthropic.com/v1/files).
//!
//! Keeps a persisted local index mapping a caller-supplied key (the cloud
//! file-store id) to the uploaded Anthropic file id, the SHA-256 of the
//! uploaded content and the time the file was last used in a query. The
//! index allows re-using an already uploaded file, replacing it when the
//! stored content changed and evicting the least recently used file when
//! the remote store would exceed its configured capacity.
//!
//! All public methods are thread-safe and blocking; call them from a worker
//! thread, never from the server event loop.
class RAgentFileStore
{

    protected:

        //! One uploaded file tracked in the local index.
        struct IndexEntry
        {
            //! Anthropic file id ("file_...").
            QString remoteFileId;
            //! SHA-256 (hex) of the uploaded content.
            QString sha256;
            //! Size of the uploaded content in bytes.
            qint64 size = 0;
            //! Last time the file was used in a query (UTC).
            QDateTime lastAccess;
        };

        //! File metadata returned by the remote list endpoint.
        struct RemoteFile
        {
            QString id;
            qint64 sizeBytes = 0;
            QDateTime createdAt;
        };

        //! Agent settings providing the API key, endpoint and network options.
        RAgentSettings settings;
        //! Maximum number of bytes kept in the remote store.
        qint64 maxStorageSize;
        //! Path of the persisted index file (empty = in-memory only).
        QString indexFileName;
        //! Local index keyed by the caller-supplied key.
        QMap<QString,IndexEntry> index;
        //! Serializes all remote operations and index access.
        QMutex mutex;
        //! Whether the index file has been loaded.
        bool indexLoaded;
        //! Last known content of the remote store (valid while remoteFilesListedAt is recent).
        QList<RemoteFile> remoteFilesCache;
        //! When remoteFilesCache was last fetched from the API (invalid = no cache).
        QDateTime remoteFilesListedAt;
        //! Set by invalidateRemoteFileListing() without taking the mutex.
        std::atomic<bool> remoteFilesListingInvalidated{false};

    public:

        //! Anthropic per-organization storage limit (100 GB); used as the default capacity.
        static const qint64 defaultMaxStorageSize;
        //! Anthropic per-file size limit (500 MB).
        static const qint64 maxRemoteFileSize;
        //! How long the remote file listing is re-used before it is fetched again.
        static const qint64 remoteFilesCacheTtlSecs;

        //! Constructor.
        explicit RAgentFileStore(const RAgentSettings &settings);

        //! Set new agent settings (API key, endpoint, proxy).
        void setSettings(const RAgentSettings &settings);

        //! Set the maximum number of bytes kept in the remote store.
        void setMaxStorageSize(qint64 maxStorageSize);

        //! Set the path of the file used to persist the index.
        void setIndexFileName(const QString &indexFileName);

        //! Ensure the given content is uploaded to the Anthropic file store.
        //!
        //! \param key       Stable identifier of the source file (cloud file-store id).
        //! \param fileName  File name reported to the API.
        //! \param content   File content.
        //! \param mimeType  Content mime type (e.g. "text/plain", "application/pdf").
        //!
        //! If the key is already indexed and the remote file still exists with
        //! identical content, only the access time is refreshed. If the content
        //! differs the remote file is replaced. The remote file listing used for
        //! these checks is re-used for remoteFilesCacheTtlSecs. Before every upload the remote
        //! usage is checked against the configured capacity and files with the
        //! oldest access time are deleted until the new file fits.
        //!
        //! Returns the Anthropic file id. Throws RError on failure.
        QString ensureFileUploaded(const QString &key, const QString &fileName, const QByteArray &content, const QString &mimeType);

        //! Forget the cached remote file listing, so the next call to
        //! ensureFileUploaded() verifies the remote store again. Call it when a
        //! request referencing an uploaded file fails (the file may have been
        //! removed from the remote store by someone else). Never blocks.
        void invalidateRemoteFileListing();

    private:

        //! Body of ensureFileUploaded(); called with the mutex held.
        QString ensureFileUploadedLocked(const QString &key, const QString &fileName, const QByteArray &content, const QString &mimeType);

        //! Return the remote file listing, fetching it only when the cached one is missing or expired.
        QList<RemoteFile> cachedRemoteFiles();

        //! Store the remote file listing after local changes (keeps its fetch time).
        void updateRemoteFilesCache(const QList<RemoteFile> &remoteFiles);

        //! Drop the cached remote file listing.
        void invalidateRemoteFilesCache();

        //! Load the index from the index file (once).
        void loadIndex();

        //! Persist the index to the index file.
        void saveIndex() const;

        //! Return the base URL of the files endpoint derived from the API URL.
        QString filesApiUrl() const;

        //! Send a single request to the files endpoint. Throws RError on failure.
        QByteArray sendRequest(const RHttpMessage &request) const;

        //! List all files currently present in the remote store.
        QList<RemoteFile> listRemoteFiles() const;

        //! Delete a file from the remote store.
        void deleteRemoteFile(const QString &remoteFileId) const;

        //! Upload content and return the new Anthropic file id.
        QString uploadFile(const QString &fileName, const QByteArray &content, const QString &mimeType) const;

        //! Delete oldest-accessed remote files until the new content fits.
        void makeRoom(QList<RemoteFile> &remoteFiles, qint64 requiredSize);

        //! Find the index key owning a remote file id (empty if none).
        QString keyForRemoteFileId(const QString &remoteFileId) const;

        //! Create index entry from Json.
        static IndexEntry indexEntryFromJson(const QJsonObject &json);

        //! Create Json from index entry.
        static QJsonObject indexEntryToJson(const IndexEntry &entry);

};

#endif // RAI_AGENT_FILE_STORE_H
