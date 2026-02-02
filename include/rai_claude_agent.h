#ifndef RAI_CLAUDE_AGENT_H
#define RAI_CLAUDE_AGENT_H

#include <QObject>

class RClaudeAgent : public QObject
{
    Q_OBJECT

public:
    explicit RClaudeAgent(QObject *parent = nullptr);
    ~RClaudeAgent() override;

signals:

public slots:

private:
};

#endif // RAI_CLAUDE_AGENT_H
