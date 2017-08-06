#pragma once

#include <chrono>

#include <QNetworkAccessManager>
#include <QString>
#include <QTimer>
#include <QUrl>

class JobMonitor : public QObject
{
	Q_OBJECT

public:
	enum BuildStatus { UNKNOWN=-1, SUCCESS, UNSTABLE, FAILURE, ABORTED, NOT_BUILT, BUILDING };

private:
	const QUrl m_url;
	const QString m_job_name;

	BuildStatus m_build_status;
	QString m_error;

	std::chrono::milliseconds m_poll_interval;
	QNetworkReply* m_request;
	QTimer* m_timer;

	QString m_username;
	QString m_password;

	QNetworkAccessManager network_manager;

public:
	JobMonitor(const QUrl& url, const QString& job_name);

	QUrl url() const;
	QString jobName() const;

	void setUsername(const QString& username);
	void setPassword(const QString& password);

	void start(const std::chrono::milliseconds& poll_interval);
	void stop();

signals:
	void buildStatusChanged(BuildStatus) const;
	void errorChanged(const QString&) const;

private:
	void setBuildStatus(const BuildStatus& build_status);
	void setError(const QString& message);

	void sendRequest();
	void processRequest();
};