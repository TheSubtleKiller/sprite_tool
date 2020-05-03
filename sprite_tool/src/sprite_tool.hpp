
class CSpriteTool
{
public:
	int Run();

	void SetMouseScroll(double _dX, double _dY)
	{
		m_dMouseScrollX = _dX;
		m_dMouseScrollY = _dY;
	}

protected:
	double m_dMouseScrollX = 0.0;
	double m_dMouseScrollY = 0.0;

	float m_fViewPortScale = 1.0f;
};