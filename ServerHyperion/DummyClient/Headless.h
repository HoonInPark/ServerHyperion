#pragma once

class Headless
{
public:
	Headless(int _InSimNum);
	~Headless();

	int StartTest();

private:
	const int m_SimNum;
};
