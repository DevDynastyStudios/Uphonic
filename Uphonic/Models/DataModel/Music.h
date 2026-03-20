#pragma once

struct TimeSignature
{
 	int numerator = 4;
 	int denominator = 4;

 	float BeatsPerMeasure() const
 	{
		return (float)numerator * 4.0f / (float)denominator;
 	}
};