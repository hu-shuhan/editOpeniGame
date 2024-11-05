#include "iGameColorMap.h"
IGAME_NAMESPACE_BEGIN

void ColorMap::InitColorBarWithGrayScaleType()
{
	m_ColorBar->Reset();
	m_ColorBar->SetDimension(3);
	m_ColorBar->Reserve(2);
	m_ColorBar->AddElement3(0.0, 0.0, 0.0);
	m_ColorBar->AddElement3(1.0, 1.0, 1.0);

	m_ColorRange->Reset();
	m_ColorRange->Reserve(2);
	m_ColorRange->AddValue(0.0);
	m_ColorRange->AddValue(1.0);
}
void ColorMap::InitColorBarWithBlueWhiteRedType()
{
	m_ColorBar->Reset();
	m_ColorBar->SetDimension(3);
	m_ColorBar->Reserve(3);
	m_ColorBar->AddElement3(0.2,0.25,0.75);
	m_ColorBar->AddElement3(0.85, 0.85, 0.85);
	m_ColorBar->AddElement3(0.9,0.15,0.1);

	m_ColorRange->Reset();
	m_ColorRange->Reserve(3);
	m_ColorRange->AddValue(0.0);
	m_ColorRange->AddValue(0.5);
	m_ColorRange->AddValue(1.0);
}
void ColorMap::InitColorBarWithBlueCyanGreenYellowRedType()
{
	m_ColorBar->Reset();
	m_ColorBar->SetDimension(3);
	m_ColorBar->Reserve(5);
	m_ColorBar->AddElement3(0.0, 0.0, 1.0);
	m_ColorBar->AddElement3(0.0, 1.0, 1.0);
	m_ColorBar->AddElement3(0.0, 1.0, 0.0);
	m_ColorBar->AddElement3(1.0, 1.0, 0.0);
	m_ColorBar->AddElement3(1.0, 0.0, 0.0);

	m_ColorRange->Reset();
	m_ColorRange->Reserve(5);
	m_ColorRange->AddValue(0.0);
	m_ColorRange->AddValue(0.25);
	m_ColorRange->AddValue(0.5);
	m_ColorRange->AddValue(0.75);
	m_ColorRange->AddValue(1.0);
}
void ColorMap::InitColorBarWithBlueCyanGreenYellowRedMagentaType()
{
	m_ColorBar->Reset();
	m_ColorBar->SetDimension(3);
	m_ColorBar->Reserve(6);
	m_ColorBar->AddElement3(0.0, 0.0, 1.0);
	m_ColorBar->AddElement3(0.0, 1.0, 1.0);
	m_ColorBar->AddElement3(0.0, 1.0, 0.0);
	m_ColorBar->AddElement3(1.0, 1.0, 0.0);
	m_ColorBar->AddElement3(1.0, 0.0, 0.0);
	m_ColorBar->AddElement3(1.0, 0.0, 1.0);

	m_ColorRange->Reset();
	m_ColorRange->Reserve(6);
	m_ColorRange->AddValue(0.0);
	m_ColorRange->AddValue(0.2);
	m_ColorRange->AddValue(0.4);
	m_ColorRange->AddValue(0.6);
	m_ColorRange->AddValue(0.8);
	m_ColorRange->AddValue(1.0);
}
void ColorMap::SetIndexColor(int index, float r, float g, float b)
{
	float rgb[3]{ r,g,b };
	m_ColorBar->SetElement(index, rgb);
}
void  ColorMap::SetIndexRange(int index, float& x) {
	this->m_ColorRange->SetValue(index, x);
}

void ColorMap::DeleteIndexColor(int index)
{
	FloatArray::Pointer retColor = FloatArray::New();
	FloatArray::Pointer retRange = FloatArray::New();
	retColor->SetDimension(3);
	float color[3]={0,0,0};
	int ColorBarSize=this->GetColorBarSize();
	for (int i = 0; i <= ColorBarSize; i++) {
		if (i == index)continue;
		this->m_ColorBar->GetElement(i, color);
		retColor->AddElement(color);
		retRange->AddValue(m_ColorRange->GetValue(i));
	}
	m_ColorBar = retColor;
	m_ColorRange = retRange;
}
void ColorMap::InsertIndexColor(int index, float r, float g, float b)
{
	FloatArray::Pointer ret = FloatArray::New();
	ret->SetDimension(3);
	for (int i = 0; i < index; i++) {
		float color[3];
		m_ColorBar->GetElement(i, color);
		ret->AddElement(color);
	}
	ret->AddElement3(r, g, b);
	int ColorBarSize = this->GetColorBarSize();
	for (int i = index; i <= ColorBarSize; i++) {
		float color[3];
		m_ColorBar->GetElement(i, color);
		ret->AddElement(color);
	}
	m_ColorBar = ret;
}
void ColorMap::InsertIndexRange(int index, float x)
{
	FloatArray::Pointer ret = FloatArray::New();
	for (int i = 0; i < index; i++) {
		ret->AddValue(m_ColorRange->GetValue(i));
	}
	ret->AddValue(x);
	int ColorBarSize = this->GetColorBarSize();
	for (int i = index; i <= ColorBarSize; i++) {
		ret->AddValue(m_ColorRange->GetValue(i));
	}
	m_ColorRange = ret;
}

void ColorMap::MapColor(float value, float rgb[3])
{
	int idx = 0;
	float st_v, fi_v, local_v;
	int ColorBarSize = this->GetColorBarSize();
	for (idx = 0; idx <= ColorBarSize; idx++) {
		if (value < m_ColorRange->GetValue(idx)) {
			break;
		}
	}

	float startRGB[3];
	float finalRGB[3];
	//std::cout << idx << std::endl;
	if (idx == 0) {
		m_ColorBar->GetElement(idx, startRGB);
		st_v = 0.0;
	}
	else {
		m_ColorBar->GetElement(idx - 1, startRGB);
		st_v = m_ColorRange->GetValue(idx - 1);
	}
	if (idx == ColorBarSize + 1) {
		m_ColorBar->GetElement(ColorBarSize, finalRGB);
		fi_v = 1.0;
	}
	else {
		m_ColorBar->GetElement(idx, finalRGB);
		fi_v = m_ColorRange->GetValue(idx);
	}
	local_v = (value - st_v) / (fi_v - st_v);


	//std::cout << st_v << " " << fi_v << " " << local_v << std::endl;
	//std::cout << startRGB[0] << " " << startRGB[1] << " " << startRGB[2] << "\n";
	//std::cout << finalRGB[0] << " " << finalRGB[1] << " " << finalRGB[2] << "\n";
	rgb[0] = local_v * finalRGB[0] + (1 - local_v) * startRGB[0];
	rgb[1] = local_v * finalRGB[1] + (1 - local_v) * startRGB[1];
	rgb[2] = local_v * finalRGB[2] + (1 - local_v) * startRGB[2];
	//std::cout << rgb[0] << " " << rgb[1] << " " << rgb[2] << "\n";
}

IGAME_NAMESPACE_END
