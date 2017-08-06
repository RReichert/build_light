#include "gpio.h"
#include "job_monitor.h"

#include <iostream>
#include <QCommandLineParser>
#include <QCoreApplication>

void info(const QString& message)
{
	std::clog << "info: " << message.toStdString() << '\n';
}

void warning(const QString& message)
{
	std::cerr << "error: " << message.toStdString() << '\n';
}

void failure(const QString& message)
{
	warning(message);
	exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
	// setup application
	QCoreApplication application(argc, argv);
	QCoreApplication::setApplicationName("Build Light");
	QCoreApplication::setApplicationVersion("1.0");

	// process command line arguments
	QCommandLineParser parser;

	parser.addHelpOption();
	parser.addVersionOption();
	parser.addPositionalArgument("job", "Name of job to monitor");
	parser.addOption(QCommandLineOption("host",       "Jenkins host", "host", "localhost"));
	parser.addOption(QCommandLineOption("port",       "Jenkins port", "port", "8080"));
	parser.addOption(QCommandLineOption("username",   "Jenkins username", "username"));
	parser.addOption(QCommandLineOption("password",   "Jenkins password", "password"));
	parser.addOption(QCommandLineOption("green_pin",  "Green pin",  "pin", "12"));
	parser.addOption(QCommandLineOption("yellow_pin", "Yellow pin", "pin", "13"));
	parser.addOption(QCommandLineOption("red_pin",    "Red pin",    "pin", "14"));

	parser.process(application);

	// validate command line arguments
	bool ok = true;

	if (parser.positionalArguments().length() != 1)
	{
		failure("user needs to specify a job name");
	}

	if (parser.value("port").toInt(&ok) && !ok)
	{
		failure("user incorrectly specified a port number");
	}

	if (parser.value("green_pin").toInt(&ok) && !ok)
	{
		failure("user has specified an invalid green pin value");
	}

	if (parser.value("yellow_pin").toInt(&ok) && !ok)
	{
		failure("user has specified an invalid yellow pin value");
	}

	if (parser.value("red_pin").toInt(&ok) && !ok)
	{
		failure("user has specified an invalid red pin value");
	}

	// initialize IO pins
	GPIO green  (parser.value("green_pin").toInt(),  GPIO::Direction::IN);
	GPIO yellow (parser.value("yellow_pin").toInt(), GPIO::Direction::IN);
	GPIO red    (parser.value("red_pin").toInt(),    GPIO::Direction::IN);

	GPIO* lights[] = {&green, &yellow, &red};

	for (GPIO* gpio : lights)
	{
		if (!gpio->open())
		{
			failure(QString("GPIO pin %1 failed (%2)")
				.arg(gpio->getPin())
				.arg(gpio->getError()
			));
		}
	}

	// startup jenkins monitor
	QUrl url;
	url.setScheme("http");
	url.setHost(parser.value("host"));
	url.setPort(parser.value("port").toInt());

	JobMonitor job_monitor(url, parser.positionalArguments()[0]);

	if (parser.isSet("username") && parser.isSet("password"))
	{
		job_monitor.setUsername(parser.value("username"));
		job_monitor.setPassword(parser.value("password"));
	}

	QObject::connect(&job_monitor, &JobMonitor::errorChanged, [&job_monitor](const QString& error)
	{
		warning(error);
	});

	QObject::connect(&job_monitor, &JobMonitor::buildStatusChanged, [&green, &yellow, &red, &lights](const JobMonitor::BuildStatus& build_status)
	{
		info(QStringLiteral("build status \"%1\"").arg(build_status));

		for (GPIO* gpio : lights)
		{
			gpio->setValue(GPIO::OFF);
		}

		switch (build_status)
		{
			case JobMonitor::SUCCESS:
				green.setValue(GPIO::ON);
				break;
			case JobMonitor::UNSTABLE:
				yellow.setValue(GPIO::ON);
				break;
			case JobMonitor::FAILURE:
				red.setValue(GPIO::ON);
				break;
			default:
				for (GPIO* gpio : lights)
				{
					gpio->setValue(GPIO::ON);
				}
		}
	});

	info(QStringLiteral("monitoring job \"%1\" (%2)")
		.arg(job_monitor.jobName())
		.arg(job_monitor.url().toString())
	);

	job_monitor.start(std::chrono::seconds(1));

	return application.exec();
}
