#include "IQWidgets/igQtModelClipWidget.h"

igQtModelClipWidget::igQtModelClipWidget(QWidget* parent)
	: QWidget(parent), ui(new Ui::Form) {

	ui->setupUi(this);
	m_Clipper = iGame::ModelClip::New();
}


void igQtModelClipWidget::SetPlane(float o[3], float n[3])
{
	m_Clipper->SetPlane(o, n);
}

void igQtModelClipWidget::UpdatePlane()
{
	float o[3] = { 0 };
	float n[3] = { 0 };

	o[0] = .0;
	o[1] = .0;
	o[2] = .0;
	n[0] = .0;
	n[1] = .0;
	n[2] = .0;

	m_Clipper->SetPlane(o, n);
}
void igQtModelClipWidget::SetIsSlice(bool s)
{
	m_Clipper->SetIsSlice(s);
}
void igQtModelClipWidget::ClipModel()
{

}