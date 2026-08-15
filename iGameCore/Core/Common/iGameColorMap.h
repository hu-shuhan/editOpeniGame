#ifndef iGameColorMap_h
#define iGameColorMap_h

#include "iGameFlatArray.h"

IGAME_NAMESPACE_BEGIN
#define IG_MAPPER_RGB_LINER 0
#define IG_MAPPER_RGB_STEP 1
class ColorMap : public Object {
public:
    I_OBJECT(ColorMap);
    static Pointer New() { return new ColorMap; }

    /*map color to rgb,the value is rescaled to 0.0-1.0*/
    void MapColor(float value, float rgb[3]);
    /*change the color in index position*/
    void SetIndexColor(int index, float r, float g, float b);
    void SetIndexColor(int index, float rgb[3]) { SetIndexColor(index, rgb[0], rgb[1], rgb[2]); }

    /*change the range in index position*/
    void SetIndexRange(int index, float& x);

    /*delete the color in index position*/
    void DeleteIndexColor(int index);

    /*add the color and range in index position*/
    void InsetIndexColorBar(int index, float x, float r, float g, float b) {
        InsertIndexColor(index, r, g, b);
        InsertIndexRange(index, x);
    }
    /*add the color and range in index position*/
    void InsetIndexColorBar(int index, float x, float rgb[3]) {
        InsertIndexColor(index, rgb);
        InsertIndexRange(index, x);
    }
    /*init colorbar with default mode,such as white-black,blue-red*/
    void InitColorBarWithGrayScaleType();
    void InitColorBarWithBlueWhiteRedType();
    void InitColorBarWithBlueCyanGreenYellowRedType();
    void InitColorBarWithBlueCyanGreenYellowRedMagentaType();

    /*get colorbar*/
    FloatArray::Pointer GetColorBar() { return this->m_ColorBar; };
    /*get color range*/
    FloatArray::Pointer GetColorRange() { return this->m_ColorRange; };
    /*init colorbar with default mode,use size to control*/
    void ResetColorBar(int size) {
        switch (size) {
            case 1:
                InitColorBarWithGrayScaleType();
                break;
            case 2:
                InitColorBarWithBlueWhiteRedType();
                break;
            case 4:
                InitColorBarWithBlueCyanGreenYellowRedType();
                break;
            case 5:
                InitColorBarWithBlueCyanGreenYellowRedMagentaType();
                break;
            default:
                break;
        }
    }
    /*update full colorbar,will update the data address rather than deeply copy*/
    void SetColorMap(FloatArray::Pointer colorbar, FloatArray::Pointer colorrange) {
        int colorbarsize = colorbar->GetNumberOfElements();
        int colorrangesize = colorrange->GetNumberOfElements();
        assert(colorbarsize == colorrangesize);
        assert(colorbar->GetDimension() == 3);
        assert(colorrange->GetDimension() == 1);
        SetColorBar(colorbar);
        SetColorRange(colorrange);
    }

    // get the colorbar draw info,use to draw colorbar
    FloatArray::Pointer GetColorBarDrawInfo();
    /*return the colorbar's size,Note that the number of segments is returned,
	not the size of the array*/
    int GetColorBarSize() { return this->m_ColorBar->GetNumberOfElements() - 1; }

    //Set map type, IG_MAPPER_RGB_LINER means liner map
    void SetMapTypeToRGBLiner() { this->m_MapType = IG_MAPPER_RGB_LINER; }
    //Set map type, IG_MAPPER_RGB_STEP means step map
    void SetMapTypeToRGBSTEP() { this->m_MapType = IG_MAPPER_RGB_STEP; }

    bool DeepCopy(const ColorMap::Pointer other);

    void MapOpacity(float value, float& opacity);

protected:
    ColorMap();
    ~ColorMap();

    FloatArray::Pointer m_ColorBar = {nullptr};
    FloatArray::Pointer m_ColorRange = {nullptr};
    int m_MapType = IG_MAPPER_RGB_LINER;

private:
    void SetColorBar(FloatArray::Pointer colorbar) { this->m_ColorBar = colorbar; };
    void SetColorRange(FloatArray::Pointer colorrange) { this->m_ColorRange = colorrange; };
    /*add the color in index position,
	and the colorbar's size will be add when range is added together*/
    void InsertIndexColor(int index, float r, float g, float b);
    void InsertIndexColor(int index, float rgb[3]) { InsertIndexColor(index, rgb[0], rgb[1], rgb[2]); }
    /*add the range in index position,
	  and the colorbar's size will be add when color is added together*/
    void InsertIndexRange(int index, float x);
};
IGAME_NAMESPACE_END
#endif
