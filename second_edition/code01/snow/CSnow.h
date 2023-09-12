#pragma once


class CSnow
{
	friend CSnow MakeSnow(int maxWidth);
protected:
	CSnow(int x, int y, int speed, int layers);
public:
	CSnow(const CSnow& snow);
	~CSnow();
	POINT& GetPosition() { return m_pos; }
	const POINT& GetPosition() const { return m_pos; }
	int GetSpeed() const { return m_speed; }
	int GetLayers() const { return m_layers; }
	int DecreaseLayers() { return --m_layers; }
	void UpdatePosition(int dps = 0);
protected:
	POINT m_pos;
	int m_speed;
	int m_layers;
};	

CSnow MakeSnow(int maxWidth);

