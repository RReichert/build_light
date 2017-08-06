#include "gpio.h"

#include <QFile>
#include <QTextStream>

GPIO::GPIO(unsigned short pin, GPIO::Direction direction) :
	m_pin(pin),
	m_direction(direction)
{

}

bool GPIO::open()
{
	QFile export_file("/sys/class/gpio/export");

	if (!export_file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		m_error = export_file.errorString();
		return false;
	}

	QTextStream export_stream(&export_file);
	export_stream << m_pin;
	export_stream.flush();

	QFile direction_file(QStringLiteral("/sys/class/gpio/gpio%1/direction").arg(m_pin));

	if (!direction_file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		m_error = direction_file.errorString();
		return false;
	}

	QTextStream direction_stream(&direction_file);
	direction_stream << (m_direction == Direction::IN ? "in" : "out");
	direction_stream.flush();

	return true;
}

void GPIO::setValue(GPIO::Value value)
{
	QFile value_file(QStringLiteral("/sys/class/gpio/gpio%1/value").arg(m_pin));

	if (!value_file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		m_error = value_file.errorString();
		return;
	}

	QTextStream value_stream(&value_file);
	value_stream << (value == Value::ON);
	value_stream.flush();
}

GPIO::Value GPIO::getValue()
{
	QFile value_file(QStringLiteral("/sys/class/gpio/gpio%1/value").arg(m_pin));

	if (!value_file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		m_error = value_file.errorString();
		return GPIO::Value::OFF;
	}

	QTextStream value_stream(&value_file);

	unsigned short value;
	value_stream >> value;

	return value ? Value::ON : Value::OFF;
}

unsigned short GPIO::getPin() const
{
	return m_pin;
}

GPIO::Direction GPIO::getDirection() const
{
	return m_direction;
}

bool GPIO::hasError() const
{
	return !m_error.isEmpty();
}

QString GPIO::getError() const
{
	return m_error;
}
