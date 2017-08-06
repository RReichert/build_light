#include "job_monitor.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>

JobMonitor::JobMonitor(const QUrl& url, const QString& job_name) : QObject(),
	m_url(url),
	m_job_name(job_name),
	m_build_status(BuildStatus::UNKNOWN),
	m_poll_interval(0),
	m_request(nullptr),
	m_timer(nullptr)
{

}

QUrl JobMonitor::url() const
{
	return m_url;
}

QString JobMonitor::jobName() const
{
	return m_job_name;
}

void JobMonitor::setUsername(const QString& username)
{
	m_username = username;
}

void JobMonitor::setPassword(const QString& password)
{
	m_password = password;
}

void JobMonitor::start(const std::chrono::milliseconds& poll_interval)
{
	stop();

	m_poll_interval = poll_interval;

	sendRequest();
}

void JobMonitor::stop()
{
	if (m_request)
	{
		delete m_request;
	}

	if (m_timer)
	{
		delete m_timer;
	}

	m_poll_interval = std::chrono::milliseconds(0);
	m_request       = nullptr;
	m_timer         = nullptr;
}

void JobMonitor::setBuildStatus(const JobMonitor::BuildStatus& build_status)
{
	if (build_status != m_build_status)
	{
		m_build_status = build_status;
		emit buildStatusChanged(m_build_status);
	}
}

void JobMonitor::setError(const QString& message)
{
	if (QString::compare(m_error, message))
	{
		m_error = message;
		emit errorChanged(m_error);
	}
}

void JobMonitor::sendRequest()
{
	QUrlQuery request_url_query;
	request_url_query.addQueryItem("tree", "jobs[name,lastBuild[building,result]]");

	QUrl request_url(m_url);
	request_url.setPath("/api/json");
	request_url.setQuery(request_url_query);

	QNetworkRequest request;
	request.setUrl(request_url);

	if (!m_username.isEmpty())
	{
		request.setRawHeader("Authorization", "Basic " + QStringLiteral("%1:%2").arg(m_username).arg(m_password).toUtf8().toBase64());
	}

	m_request = network_manager.get(request);

	connect(m_request, &QNetworkReply::finished, this, &JobMonitor::processRequest);
}

void JobMonitor::processRequest()
{
	if (m_request->error())
	{
		setError(m_request->errorString());
	}

	else
	{
		QJsonDocument document = QJsonDocument::fromJson(m_request->readAll());
		QJsonArray jobs        = document.object()["jobs"].toArray();

		QStringList job_names;

		QJsonArray::iterator itr = std::find_if(jobs.begin(), jobs.end(), [this, &job_names](const QJsonValueRef& job)
		{
			QString job_name = job.toObject()["name"].toString();
			job_names << job_name;

			return !QString::compare(job_name, m_job_name);
		});

		if (itr != jobs.end())
		{
			QJsonObject last_build = itr->toObject()["lastBuild"].toObject();

			bool building  = last_build["building"].toBool();
			QString result = last_build["result"].toString();

			const QPair<const char*, BuildStatus> result_map[] =
			{
				{ "SUCCESS",   BuildStatus::SUCCESS   },
				{ "UNSTABLE",  BuildStatus::UNSTABLE  },
				{ "FAILURE",   BuildStatus::FAILURE   },
				{ "ABORTED",   BuildStatus::ABORTED   },
				{ "NOT_BUILT", BuildStatus::NOT_BUILT }
			};

			BuildStatus build_status = BuildStatus::UNKNOWN;

			if (building)
			{
				build_status = BuildStatus::BUILDING;
			}
			else
			{
				for (const auto& entry : result_map)
				{
					if (!QString::compare(result, entry.first))
					{
						build_status = entry.second;
					}
				}
			}

			if (build_status == BuildStatus::UNKNOWN)
			{
				setError(QStringLiteral("unknown job result of \"%1\"")
					.arg(result)
				);
			}

			setBuildStatus(build_status);
		}

		else
		{
			setError(QStringLiteral("no job found by the name of \"%1\", list of available jobs:\n - %2")
				.arg(m_job_name)
				.arg(job_names.join("\n - "))
			);
		}
	}

	if (!m_timer)
	{
		m_timer = new QTimer(this);
		m_timer->setSingleShot(true);
		m_timer->setInterval(m_poll_interval.count());
		connect(m_timer, &QTimer::timeout, [this](){ sendRequest(); });
	}

	m_request->deleteLater();
	m_request = nullptr;

	m_timer->start();
}