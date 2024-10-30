#include "iGameColorMap.h"
IGAME_NAMESPACE_BEGIN

void ColorMap::InitColorBarByOneSegmentType()
{
	ColorBar->Reset();
	ColorBar->SetDimension(3);
	ColorBar->Reserve(2);
	ColorBar->AddElement3(0.0, 0.0, 0.0);
	ColorBar->AddElement3(1.0, 1.0, 1.0);

	ColorRange->Reset();
	ColorRange->Reserve(2);
	ColorRange->AddValue(0.0);
	ColorRange->AddValue(1.0);
	ColorBarSize = 1;
}
void ColorMap::InitColorBarByTwoSegmentType()
{
	ColorBar->Reset();
	ColorBar->SetDimension(3);
	ColorBar->Reserve(3);
	ColorBar->AddElement3(0.2,0.25,0.75);
	ColorBar->AddElement3(0.85, 0.85, 0.85);
	ColorBar->AddElement3(0.9,0.15,0.1);

	ColorRange->Reset();
	ColorRange->Reserve(3);
	ColorRange->AddValue(0.0);
	ColorRange->AddValue(0.5);
	ColorRange->AddValue(1.0);
	ColorBarSize = 2;
}
void ColorMap::InitColorBarByFourSegmentType()
{
	ColorBar->Reset();
	ColorBar->SetDimension(3);
	ColorBar->Reserve(5);
	ColorBar->AddElement3(0.0, 0.0, 1.0);
	ColorBar->AddElement3(0.0, 1.0, 1.0);
	ColorBar->AddElement3(0.0, 1.0, 0.0);
	ColorBar->AddElement3(1.0, 1.0, 0.0);
	ColorBar->AddElement3(1.0, 0.0, 0.0);

	ColorRange->Reset();
	ColorRange->Reserve(5);
	ColorRange->AddValue(0.0);
	ColorRange->AddValue(0.25);
	ColorRange->AddValue(0.5);
	ColorRange->AddValue(0.75);
	ColorRange->AddValue(1.0);
	ColorBarSize = 4;
}
void ColorMap::InitColorBarByFiveSegmentType()
{
	ColorBar->Reset();
	ColorBar->SetDimension(3);
	ColorBar->Reserve(6);
	ColorBar->AddElement3(0.0, 0.0, 1.0);
	ColorBar->AddElement3(0.0, 1.0, 1.0);
	ColorBar->AddElement3(0.0, 1.0, 0.0);
	ColorBar->AddElement3(1.0, 1.0, 0.0);
	ColorBar->AddElement3(1.0, 0.0, 0.0);
	ColorBar->AddElement3(1.0, 0.0, 1.0);

	ColorRange->Reset();
	ColorRange->Reserve(6);
	ColorRange->AddValue(0.0);
	ColorRange->AddValue(0.2);
	ColorRange->AddValue(0.4);
	ColorRange->AddValue(0.6);
	ColorRange->AddValue(0.8);
	ColorRange->AddValue(1.0);
	ColorBarSize = 5;
}
void ColorMap::SetColorBar(int index, float r, float g, float b)
{
	float rgb[3]{ r,g,b };
	ColorBar->SetElement(index, rgb);
}
void  ColorMap::SetColorRange(int index, float& x) {
	this->ColorRange->SetValue(index, x);
}

void ColorMap::DeleteIndexColor(int index)
{
	FloatArray::Pointer retColor = FloatArray::New();
	FloatArray::Pointer retRange = FloatArray::New();
	retColor->SetDimension(3);
	float color[3];
	for (int i = 0; i <= ColorBarSize; i++) {
		if (i == index)continue;
		this->ColorBar->GetElement(i, color);
		retColor->AddElement(color);
		retRange->AddValue(ColorRange->GetValue(i));
	}
	ColorBar = retColor;
	ColorRange = retRange;
	ColorBarSize--;
}
void ColorMap::AddColorBar(int index, float r, float g, float b)
{
	FloatArray::Pointer ret = FloatArray::New();
	ret->SetDimension(3);
	for (int i = 0; i < index; i++) {
		float color[3];
		ColorBar->GetElement(i, color);
		ret->AddElement(color);
	}
	ret->AddElement3(r, g, b);
	for (int i = index; i <= ColorBarSize; i++) {
		float color[3];
		ColorBar->GetElement(i, color);
		ret->AddElement(color);
	}
	ColorBarSize = std::min(ret->GetNumberOfElements() - 1, this->ColorRange->GetNumberOfElements() - 1);
	ColorBar = ret;
}
void ColorMap::AddColorRange(int index, float x)
{
	FloatArray::Pointer ret = FloatArray::New();
	for (int i = 0; i < index; i++) {
		ret->AddValue(ColorRange->GetValue(i));
	}
	ret->AddValue(x);
	for (int i = index; i <= ColorBarSize; i++) {
		ret->AddValue(ColorRange->GetValue(i));
	}
	ColorBarSize = std::min(ret->GetNumberOfElements() - 1, this->ColorBar->GetNumberOfElements() - 1);
	ColorRange = ret;
}

void ColorMap::MapColor(float value, float rgb[3])
{
	int idx = 0;
	float st_v, fi_v, local_v;
	for (idx = 0; idx <= ColorBarSize; idx++) {
		if (value < ColorRange->GetValue(idx)) {
			break;
		}
	}

	float startRGB[3];
	float finalRGB[3];
	//std::cout << idx << std::endl;
	ColorBar->SetDimension(3);
	if (idx == 0) {
		ColorBar->GetElement(idx, startRGB);
		st_v = 0.0;
	}
	else {
		ColorBar->GetElement(idx - 1, startRGB);
		st_v = ColorRange->GetValue(idx - 1);
	}
	if (idx == ColorBarSize + 1) {
		ColorBar->GetElement(ColorBarSize, finalRGB);
		fi_v = 1.0;
	}
	else {
		ColorBar->GetElement(idx, finalRGB);
		fi_v = ColorRange->GetValue(idx);
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
FloatArray::Pointer ColorMap::GetColorBarDrawInfo()
{
	FloatArray::Pointer ret = FloatArray::New();
	ret->SetDimension(3);
	float color[16];
	for (int i = 0; i < ColorBarSize; i++) {
		ColorBar->GetElement(i, color);
		ret->AddElement3(-0.95f, ColorRange->GetValue(i) - 0.9, 0.0);
		ret->AddElement3(color[0], color[1], color[2]);
		ret->AddElement3(-0.90f, ColorRange->GetValue(i) - 0.9, 0.0);
		ret->AddElement3(color[0], color[1], color[2]);

		ColorBar->GetElement(i + 1, color);
		ret->AddElement3(-0.90, ColorRange->GetValue(i + 1) - 0.9, 0.0);
		ret->AddElement3(color[0], color[1], color[2]);
		ret->AddElement3(-0.95, ColorRange->GetValue(i + 1) - 0.9, 0.0);
		ret->AddElement3(color[0], color[1], color[2]);
	}
	return ret;
}

IGAME_NAMESPACE_END
