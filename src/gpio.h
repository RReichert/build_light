#pragma once

#include <QString>

class GPIO
{
public:
	enum Value { OFF, ON };
	enum Direction { IN, OUT };

private:
	const unsigned short m_pin;
	const Direction m_direction;

	QString m_error;

public:
	GPIO(unsigned short pin, Direction direction);
	bool open();

	void setValue(Value value);
	Value getValue();

	unsigned short getPin() const;
	Direction getDirection() const;

	bool hasError() const;
	QString getError() const;
};