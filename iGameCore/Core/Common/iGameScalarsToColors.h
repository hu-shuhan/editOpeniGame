#ifndef iGameScalarsToColors_h
#define iGameScalarsToColors_h

#include "iGameFlatArray.h"
#include "iGameColorMap.h"

IGAME_NAMESPACE_BEGIN

class ScalarsToColors : public ColorMap {
public:
	I_OBJECT(ScalarsToColors);
	static Pointer New() { return new ScalarsToColors; }

	/*Get the range of scaled data*/
	virtual float* GetRange();
	virtual void InitRange(ArrayObject::Pointer input) {
		if(this->VectorMode== RGBCOLORS)return;
		this->SetVectorModeToMagnitude();
		InitRange(input, 0, -1);
	}
	/*Init the range of scaled data,the data is input,the component is the scaled component*/
	virtual void InitRange(ArrayObject::Pointer input, int component) {
		if (this->VectorMode == RGBCOLORS)return;
		if (component < 0) {
			return InitRange(input);
		}
		this->SetVectorModeToComponent();
		InitRange(input, component, 1);
	}
	/*Init the range of scaled data,the data is input,
	the component is the scaled component,the size is the length to scale*/
	virtual void InitRange(ArrayObject::Pointer input, int component, int size);
	/*Set the range of scaled data*/
	virtual void SetRange(float min, float max);
	virtual void SetRange(const float rng[2]) { this->SetRange(rng[0], rng[1]); }
	/*Map the value to rgb,return the unsignend char array,means the rgb data with range 0-255
	  note the value is not scaled,so you need give the shift and scale to scale the value*/
	virtual const unsigned char* MapValue(float v, float& shift, float& scale);
	/*Map the value to rgb,return the float array,means the rgb data with range 0.0-1.0
	  note the value is not scaled,so you need give the shift and scale to scale the value*/
	virtual const float* MapValueToRGB(float v, float& shift, float& scale);
	virtual void MapValueToRGB(float v, float* rgb, float& shift, float& scale);
	/*Compute the shift and scale*/
	void ComputeShiftScale(float& shift, float& scale);
	/*Get Color with the value, shift and scale*/
	virtual void GetColor(float v, float rgb[3], float& shift, float& scale);
	virtual void GetColor(float v, float rgb[3]) {
		float shift = 0.0, scale = 1.0;
		this->ComputeShiftScale(shift, scale);
		GetColor(v, rgb, shift, scale);
	}
	/*Get the opacity of value v,temporarily fixed return 1.0 */
	virtual float GetOpacity(float v) { return 1.0; };
	/*Set the opacity*/
	virtual void SetAlpha(float alpha);
	/*Get the luminance with no scaled value x,note the value is not scaled,
	so you need give the shift and scale to scale the value*/
	float GetLuminance(float x, float& shift, float& scale)
	{
		float rgb[3];
		this->GetColor(x, rgb, shift, scale);
		return static_cast<float>(rgb[0] * 0.30 + rgb[1] * 0.59 + rgb[2] * 0.11);
	}
	/*Map scalars, return the color array with float type,
	the component is the dimension that needs to be mapped,
	the outputFormat is the color's dimension that return,
	3 means the rgb, 4 means the rgba, 1 means the Grayscale
	*/
	virtual FloatArray::Pointer MapScalars(
		ArrayObject::Pointer scalars, int component, int outputFormat = 3);
	virtual FloatArray::Pointer MapScalars(ArrayObject::Pointer scalars) {
		return MapScalars(scalars, -1, 3);
	}
	/*Map scalars, return the color array with unsigned char type,
	the component is the dimension that needs to be mapped,
	the outputFormat is the color's dimension that return,
	3 means the rgb, 4 means the rgba, 1 means the Grayscale
	*/
	//virtual iGameUnsignedCharArray* MapScalars(
	//	iGameAbstractArray* scalars, int component, int outputFormat = 3);

	/*Set map mode, component means the single-dimensional,
	* magnitude means the mixed-dimensional, need compute the magnitue
	* RGBColors means the data is rgb color, didn't need to compute value
	*/
	void SetVectorModeToMagnitude();
	void SetVectorModeToComponent();
	void SetVectorModeToRGBColors();
	void SetVectorMode(int m) { this->VectorMode = m; };
	enum VectorModes
	{
		MAGNITUDE = 0,
		COMPONENT = 1,
		RGBCOLORS = 2
	};


	/*vectorComponent means the start dimension,
	vectorSize means the size to compute,for example,
	vectorComponent=0，vectorSize=1 means the first dimension to compute color*/
	void MapVectorsThroughTable(ArrayObject::Pointer input, FloatArray::Pointer output, int outputFormat,
		int vectorComponent, int vectorSize);
	void MapVectorsThroughTable(ArrayObject::Pointer input, FloatArray::Pointer output, int outputFormat) {
		this->MapVectorsThroughTable(input, output, outputFormat, -1, -1);
	}
	/*change real rgb (range 0.0 to 1.0)to unsigned char type (0,255)*/
	template <typename T>
	static unsigned char ColorToUChar(T t)
	{
		return static_cast<unsigned char>(t);
	}
	template <typename T>
	static void ColorToUChar(T t, unsigned char* dest) {
		*dest = ColorToUChar(t);
	}
	/*Get the info with compute data*/
	int GetVectorMode() { return this->VectorMode; };
	int GetVectorComponent() { return this->VectorComponent; };
	int GetVectorSize() { return this->VectorSize; };

protected:
	ScalarsToColors();
	~ScalarsToColors() override;

	unsigned char RGBABytes[4];
	float Alpha;

	int VectorMode;
	int VectorComponent;
	int VectorSize;

private:
	float RGB[3];
	float InputRange[2];
};

template <>
inline unsigned char ScalarsToColors::ColorToUChar(double t)
{
	double temp = (t * 255.0) + 0.5;
	return static_cast<unsigned char>(temp);
}
template <>
inline unsigned char ScalarsToColors::ColorToUChar(float t)
{
	float temp = (t * 255.0) + 0.5;
	return static_cast<unsigned char>(temp);
}

IGAME_NAMESPACE_END
#endif