#include "IQWidgets/igQtModelClipWidget.h"

igQtModelClipWidget::igQtModelClipWidget(QWidget* parent)
    : QWidget(parent)/*, ui(new Ui::ScalarView) */{

	m_Clipper=iGame::ModelClip::New();
}


void igQtModelClipWidget::SetPlane(float n[3], float o[3])
{
	m_Clipper->SetPlane(n,o);
}

void igQtModelClipWidget::UpdatePlane()
{

}
void igQtModelClipWidget::SetIsSlice(bool s)
{
	m_Clipper->SetIsSlice(s);
}
void igQtModelClipWidget::ClipModel()
{

}