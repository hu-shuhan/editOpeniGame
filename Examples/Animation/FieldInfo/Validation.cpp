// Fix commit subject: 修复动画 Filter 字段传递与播放颜色映射
// Tests and fixes are introduced together; resolve the commit with:
// git log --diff-filter=A --format="%h %s" -- Examples/Animation/FieldInfo/Validation.cpp
// Regression (2026-09-28, commit: see above): downstream parameter dialogs read the
// original cell fields, so CellToPoint -> IsoVolume could not be configured.
// Infer schemas without calling execute or changing arrays. Include multi-block
// intersections, duplicate names, unknown adapters and actual two-frame output.
#include <IQCore/igQtAnimationFilterAdapters.h>
#include <IQCore/igQtAnimationFilterManager.h>
#include <IQCore/igQtAnimationPipeline.h>
#include <IQCore/igQtAnimationFrameSource.h>
#include <IQCore/igQtAnimationVcrController.h>
#include <IQWidgets/igQtAnimationWidget.h>
#include <IQCore/igQtMainWindow.h>
#include <QSurfaceFormat>
#include <QEventLoop>
#include <QTimer>
#include <IQWidgets/igQtScalarViewWidget.h>
#include <IQComponents/igQtModelDialogWidget.h>
#include <IQComponents/igQtFilterDialogDockWidget.h>
#include <iGameFileIO.h>
#include <iGameSceneManager.h>
#include <iGameUnstructuredMesh.h>
#include <Log/iGameLogger.h>
#include <QApplication>
#include <QTemporaryDir>
#include <QLabel>
#include <QImage>
#include <QPushButton>
#include <GLFW/glfw3.h>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <cmath>
using namespace iGame;
void Check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
UnstructuredMesh::Pointer Frame(double value) {
    auto mesh=UnstructuredMesh::New(); auto points=Points::New();
    points->AddPoint(0,0,0); points->AddPoint(1,0,0); points->AddPoint(0,1,0);
    points->AddPoint(0,0,1); points->AddPoint(0,0,-1); mesh->SetPoints(points);
    igIndex a[]{0,1,2,3}, b[]{0,2,1,4}; mesh->AddCell(a,4,IG_TETRA); mesh->AddCell(b,4,IG_TETRA);
    auto density=FloatArray::New(); density->SetName("density"); density->SetDimension(1);
    density->AddValue(0); density->AddValue(value); mesh->GetAttributeSet()->AddScalar(IG_CELL,density);
    auto velocity=FloatArray::New(); velocity->SetName("velocity"); velocity->SetDimension(3); velocity->Resize(2);
    for (int i=0;i<6;++i) velocity->SetValue(i,value+i);
    mesh->GetAttributeSet()->AddVector(IG_CELL,velocity); return mesh;
}
QVariantMap IsoParameters() {
    return {{"scalarName","density"},{"scalarDimension","0"},{"lowerValue",.25},{"upperValue",.75}};
}
void Descriptions() {
    DataObject::DeferDrawableConversionScope cpu;
    igQtAnimationFilterManager manager; QString error;
    Check(igQtRegisterBuiltinAnimationFilters(manager,&error),"registration");
    auto frame=Frame(2); auto original=frame->GetAttributeSet()->GetAttribute(0).pointer;
    auto range=frame->GetAttributeSet()->GetAttribute(0).dataRange;
    igQtAnimationDataInfo source, converted, result;
    Check(igQtDescribeAnimationData(frame,source,error),"source description");
    igQtAnimationFilterParameterSchema schema;
    Check(!manager.parameterSchema("isoVolume",source,schema,error),"cell field accepted before conversion");
    Check(manager.describeOutput("convertToPointData",source,{},converted,error),"conversion inference");
    Check(converted.fields[0].association==IG_POINT && converted.fields[1].components==3,"wrong inferred fields");
    Check(manager.parameterSchema("isoVolume",converted,schema,error),"downstream schema");
    Check(schema[0].choices.contains("density") && schema[0].choices.contains("velocity"),"missing converted field");
    Check(!schema[2].defaultValue.isValid() && !schema[3].defaultValue.isValid(),"invented converted range");
    Check(manager.describeOutput("isoVolume",converted,IsoParameters(),result,error),"valid iso metadata rejected");
    auto invalid=IsoParameters(); invalid["scalarDimension"]=1;
    Check(!manager.describeOutput("isoVolume",converted,invalid,result,error),"invalid component accepted");
    invalid=IsoParameters(); invalid["lowerValue"]=1.;
    Check(!manager.describeOutput("isoVolume",converted,invalid,result,error),"inverted range accepted");
    Check(manager.describeOutput("contour",converted,{{"scalarName","density"},{"scalarDimension",0},{"isoValue",.5}},result,error),"contour inference");
    igQtAnimationPipelineSteps steps{{"convertToPointData",{}},{"isoVolume",IsoParameters()}};
    Check(igQtDescribeAnimationPipelineInput(manager,steps,1,source,result,error),"prefix inference");
    Check(result.fields[0].association==IG_POINT,"wrong row input");
    Check(original==frame->GetAttributeSet()->GetAttribute(0).pointer && original->GetValue(1)==2 &&
          frame->GetAttributeSet()->GetAttribute(0).attachmentType==IG_CELL &&
          frame->GetAttributeSet()->GetAttribute(0).dataRange==range,"configuration mutated original data");
    auto duplicate=source; duplicate.fields.push_back({"density",IG_POINT,1,IG_SCALAR});
    Check(!manager.describeOutput("convertToPointData",duplicate,{},result,error),"conversion collision accepted");
    igQtAnimationDataInfo blocks; blocks.blocks={source,source}; blocks.blocks[1].fields.erase(blocks.blocks[1].fields.begin());
    Check(manager.describeOutput("convertToPointData",blocks,{},converted,error),"multi-block conversion");
    Check(manager.parameterSchema("isoVolume",converted,schema,error),"common vector missing");
    Check(!schema[0].choices.contains("density") && schema[0].choices.contains("velocity"),"first-block-only field exposed");
    Check(!manager.describeOutput("isoVolume",converted,IsoParameters(),result,error),"missing field in second block accepted");
    int executes=0; igQtAnimationFilterDescriptor legacy;
    legacy.id="legacy"; legacy.displayName="Legacy";
    legacy.execute=[&](const igQtAnimationFrameContext&,const QVariantMap&) { ++executes; return igQtAnimationFilterResult{}; };
    Check(manager.registerFilter(legacy),"legacy registration");
    Check(!manager.describeOutput("legacy",source,{},result,error) && executes==0 && !error.isEmpty(),"unknown adapter executed during inference");
    for (double value : {2.,4.}) {
        igQtAnimationFrameContext context; context.input=Frame(value);
        igQtAnimationFilterResult output;
        Check(igQtExecuteAnimationPipeline(manager,steps,context,output,&error),error.toStdString().c_str());
        auto mesh=DynamicCast<UnstructuredMesh>(output.output);
        Check(mesh && mesh->GetNumberOfCells()>0,"pipeline produced no volume");
        auto field=mesh->GetAttributeSet()->GetAttribute("density");
        Check(field.attachmentType==IG_POINT,"actual output association differs");
        for (IGsize i=0;i<field.pointer->GetNumberOfValues();++i)
            Check(field.pointer->GetValue(i)>=.25-1e-5 && field.pointer->GetValue(i)<=.75+1e-5,"iso bounds violated");
    }
}
// Regression (2026-09-28, commit: see above): configuring the second row must not
// execute conversion, and Apply must validate the inferred input too. Upstream
// edits invalidate stale dialogs even if rows have shifted to the same Filter.
template<class T> T* Control(QObject& root,const char* name) {
    auto value=root.findChild<T*>(name); Check(value!=nullptr,name); return value;
}
void DialogTest() {
    namespace fs=std::filesystem;
    QTemporaryDir directory; Check(directory.isValid(),"fixture directory");
    fs::path dir=directory.path().toStdString();
    {
        std::ofstream pvd(dir/"data.pvd"); pvd<<"<VTKFile type=\"Collection\"><Collection>";
        for (int i=0;i<2;++i) {
            pvd<<"<DataSet timestep=\""<<i<<"\" file=\"frame"<<i<<".vtu\"/>";
            std::ofstream vtu(dir/("frame"+std::to_string(i)+".vtu"));
            vtu<<"<VTKFile type=\"UnstructuredGrid\" byte_order=\"LittleEndian\"><UnstructuredGrid>"
                   "<Piece NumberOfPoints=\"5\" NumberOfCells=\"2\"><CellData>"
                   "<DataArray Name=\"density\" type=\"Float32\" format=\"ascii\">0 "<<2*(i+1)<<"</DataArray>"
                   "<DataArray Name=\"temperature\" type=\"Float32\" format=\"ascii\">10 "<<20*(i+1)<<"</DataArray></CellData>"
                   "<Points><DataArray type=\"Float32\" NumberOfComponents=\"3\" format=\"ascii\">0 0 0 1 0 0 0 1 0 0 0 1 0 0 -1</DataArray></Points>"
                   "<Cells><DataArray Name=\"connectivity\" type=\"Int32\" format=\"ascii\">0 1 2 3 0 2 1 4</DataArray>"
                   "<DataArray Name=\"offsets\" type=\"Int32\" format=\"ascii\">4 8</DataArray>"
                   "<DataArray Name=\"types\" type=\"UInt8\" format=\"ascii\">10 10</DataArray></Cells>"
                   "</Piece></UnstructuredGrid></VTKFile>";
        }
        pvd<<"</Collection></VTKFile>";
    }
    Check(glfwInit()!=0,"GLFW init"); glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,4); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,6);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    auto window=glfwCreateWindow(160,120,"Field info regression",nullptr,nullptr);
    Check(window!=nullptr,"OpenGL unavailable"); glfwMakeContextCurrent(window);
    auto scene=SceneManager::Instance()->NewScene(); scene->Initialize(); scene->Resize(160,120,1);
    scene->EnableFramePacing(false); scene->SetMakeCurrentFunctor([&] { glfwMakeContextCurrent(window); });
    auto source=FileIO::ReadFile((dir/"data.pvd").generic_string()); Check(source!=nullptr,"read PVD");
    auto id=scene->AddModel(source); scene->ResetCameraView(source->GetBoundingBox());
    // Fix commit subject: fix(animation): isolate output frame reads from legacy cache
    // Locate: git log --format="%h %s" --grep="isolate output frame reads from legacy cache" -- Examples/Animation/FieldInfo/Validation.cpp
    // Regression (2026-10-07): bypassing legacy caching must not
    // borrow cached arrays or touch its LRU order, even on interpolation/failure.
    // A private metadata copy must also preserve the original frame indices.
    {
        DataObject::DeferDrawableConversionScope cpu;
        auto probe=DrawObject::New(); auto frames=StreamingData::New();
        for (int i=0;i<3;++i) {
            auto files=StringArray::New();
            files->AddElement((dir/("frame"+std::to_string(i%2)+".vtu")).generic_string());
            frames->AddTimeStep(float(i),files,StreamingType::MultiSubFiles);
        }
        frames->AddTimeStep(3.f,StringArray::New(),StreamingType::NONE);
        probe->SetTimeFrames(frames); frames->EnableCache(2);
        auto first=frames->GetTargetTimeFrameData(0);
        auto second=frames->GetTargetTimeFrameData(1);
        auto oldValues=DynamicCast<DataObject>(first.front())->GetAttributeSet()->GetAttribute("density").pointer;
        oldValues->SetValue(1,102.);
        igQtAnimationFrameContext context; QString error;
        Check(igQtLoadAnimationFrame(probe,{0},context,error),"isolated frame read");
        auto output=context.input;
        auto values=output->SubDataObjectIteratorBegin()->second->GetAttributeSet()->GetAttribute("density").pointer;
        Check(values!=oldValues && values->GetValue(1)==2.,"animation borrowed legacy cached data");
        values->SetValue(1,999.);
        Check(oldValues->GetValue(1)==102.,"animation wrote into legacy arrays");
        auto privateFrames=output->PeekTimeFrames();
        Check(privateFrames!=frames && privateFrames->GetTimeNum()==4 && privateFrames->GetCurrentCacheCount()==0,
              "animation shared legacy time series or cached intermediate frames");
        Check(privateFrames->GetTargetTimeFrame(0).GetMetaData()!=frames->GetTargetTimeFrame(0).GetMetaData(),
              "animation shared mutable frame metadata");
        Check(igQtLoadAnimationFrame(probe,{0,true,.5f,1},context,error),"isolated interpolation");
        Check(context.input->SubDataObjectIteratorBegin()->second->GetAttributeSet()->GetAttribute("density").pointer->GetValue(1)==3.,
              "interpolation used cached source values");
        Check(!igQtLoadAnimationFrame(probe,{3},context,error),"empty frame should fail");
        Check(frames->GetMaxCacheSize()==2 && frames->GetCurrentCacheCount()==2 &&
              frames->GetTargetTimeFrame(0).GetCachedData()==first &&
              frames->GetTargetTimeFrame(1).GetCachedData()==second,"isolated reads altered legacy entries");
        frames->GetTargetTimeFrameData(2);
        Check(!frames->GetTargetTimeFrame(0).GetISCached() && frames->GetTargetTimeFrame(1).GetISCached(),
              "animation touched legacy LRU order");
        Check(frames->GetTargetTimeFrameData(1)==second,"legacy cache hit no longer works");
        // Conversely, clearing the legacy cache must leave animation data intact.
        frames->DisableCache();
        Check(values->GetValue(1)==999.,"clearing legacy cache changed animation data");
    }
    // Regression (2026-10-07, same fix commit as above): animation initialization, reads and
    // capacity changes disabled/cleared the shared legacy cache. Keep its two
    // entries and their modified data intact across playback, interpolation,
    // export, pipeline edits, model switches and widget destruction. Distinct
    // sentinel values prove animation reads disk rather than legacy objects.
    auto legacyFrames=source->PeekTimeFrames();
    legacyFrames->EnableCache(2);
    auto legacyFirst=legacyFrames->GetTargetTimeFrameData(0);
    auto legacySecond=legacyFrames->GetTargetTimeFrameData(1);
    auto legacyField=DynamicCast<DataObject>(legacyFirst.front())->GetAttributeSet()->GetAttribute("density").pointer;
    legacyField->SetValue(1,102.);
    auto checkLegacy=[&] {
        Check(legacyFrames->GetMaxCacheSize()==2 && legacyFrames->GetCurrentCacheCount()==2,
              "animation changed legacy cache capacity or entries");
        Check(legacyFrames->GetTargetTimeFrame(0).GetCachedData()==legacyFirst &&
              legacyFrames->GetTargetTimeFrame(1).GetCachedData()==legacySecond,
              "animation replaced legacy cache objects");
        Check(legacyField->GetValue(1)==102.,"animation mutated legacy cached values");
    };
    {
        igQtAnimationWidget widget; widget.initAnimationComponents();
        checkLegacy();
        QObject::connect(&widget,&igQtAnimationWidget::AnimationFrameChanged,&widget,checkLegacy);
        // Regression (2026-09-28, fix: 待提交): real main-window callbacks
        // rebuilt the attribute tree by selecting -1 after every frame, disabling
        // scalar coloring. Include both UI callbacks, not just the frame producer.
        QWidget panels;
        igQtModelDialogWidget tree(&panels);
        auto treeControl=tree.getTreeDock()->findChild<igQtModelTreeWidget*>();
        Check(treeControl!=nullptr,"model tree missing");
        auto modelRow=new ModelTreeWidgetItem(treeControl);
        modelRow->setModel(scene->GetCurrentModel());
        treeControl->addTopLevelItem(modelRow);
        igQtScalarViewWidget scalar;
        QObject::connect(&widget,&igQtAnimationWidget::AnimationDataChanged,&tree,[&] {
            Check(scene->GetCurrentModel()->GetDataObject()->PeekTimeFrames()!=legacyFrames,
                  "final output reattached legacy cache");
            tree.refreshAnimationAttributes(scene->GetCurrentModel()->GetDataObject());
        });
        QObject::connect(&widget,&igQtAnimationWidget::AnimationFrameChanged,
                         &scalar,&igQtScalarViewWidget::showScalarView);
        widget.show(); QApplication::processEvents();
        auto combo=Control<QComboBox>(widget,"comboBoxAnimationFilter");
        auto list=Control<QListWidget>(widget,"listWidgetAnimationPipeline");
        auto add=[&](const char* name) {
            combo->setCurrentIndex(combo->findData(name)); Control<QPushButton>(widget,"btnAnimationFilterAdd")->click();
        };
        auto unchanged=[&] {
            Check(source->HasSubDataObject(),"source has no loaded frame blocks");
            auto child=source->SubDataObjectIteratorBegin()->second;
            Check(child->GetAttributeSet()->GetAttribute("density").attachmentType==IG_CELL,"parameter dialog executed conversion");
        };
        add("convertToPointData"); add("isoVolume");
        unchanged();
        list->setCurrentRow(1); Control<QPushButton>(widget,"btnAnimationFilterParameters")->click();
        QApplication::processEvents();
        auto dialog=Control<igQtFilterDialogDockWidget>(widget,"animationFilterParameters");
        Check(Control<QComboBox>(*dialog,"animationParam_scalarName")->currentText()=="density","converted field not selected");
        auto lower=Control<QLineEdit>(*dialog,"animationParam_lowerValue");
        auto upper=Control<QLineEdit>(*dialog,"animationParam_upperValue");
        Check(lower->text().isEmpty() && upper->text().isEmpty(),"unknown range presented as exact");
        dialog->apply();
        Check(!Control<QLabel>(*dialog,"animationParameterError")->text().isEmpty(),"empty thresholds accepted");
        lower->setText("0.25"); upper->setText("0.75"); dialog->apply();
        Check(Control<QLabel>(*dialog,"animationParameterError")->text().isEmpty() && list->item(1)->text().contains(QStringLiteral("参数已设置")),"inferred input rejected on Apply");
        unchanged();
        Check(dialog->grab().save(QCoreApplication::applicationDirPath()+"/field-info-parameters.png"),"dialog screenshot");
        dialog->close();
        Control<QPushButton>(widget,"btnAnimationFilterParameters")->click();
        QApplication::processEvents();
        dialog=Control<igQtFilterDialogDockWidget>(widget,"animationFilterParameters");
        Check(Control<QLineEdit>(*dialog,"animationParam_upperValue")->text().toDouble()==.75,"saved parameters lost");
        // A changed source field must not silently switch an existing selection.
        dialog->close();
        auto originalField=source->SubDataObjectIteratorBegin()->second->GetAttributeSet()->GetAttribute("density").pointer;
        originalField->SetName("renamed");
        Control<QPushButton>(widget,"btnAnimationFilterParameters")->click();
        QApplication::processEvents();
        dialog=Control<igQtFilterDialogDockWidget>(widget,"animationFilterParameters");
        auto names=Control<QComboBox>(*dialog,"animationParam_scalarName");
        Check(names->findText("renamed")>=0 && names->currentText()=="density","stale field was silently replaced");
        dialog->apply();
        Check(!Control<QLabel>(*dialog,"animationParameterError")->text().isEmpty(),"missing field accepted on Apply");
        originalField->SetName("density"); dialog->close();
        Control<QPushButton>(widget,"btnAnimationFilterParameters")->click();
        QApplication::processEvents();
        dialog=Control<igQtFilterDialogDockWidget>(widget,"animationFilterParameters");
        list->setCurrentRow(0); Control<QPushButton>(widget,"btnAnimationFilterRemove")->click();
        Check(list->item(0)->text().contains(QStringLiteral("需配置")),"downstream not invalidated after conversion removed");
        dialog->apply();
        Check(!Control<QLabel>(*dialog,"animationParameterError")->text().isEmpty(),"stale parameter dialog saved into wrong row");
        dialog->close();
        add("convertToPointData"); Control<QPushButton>(widget,"btnAnimationFilterUp")->click();
        Check(list->item(1)->text().contains(QStringLiteral("参数已设置")),"reordered prefix did not recover");
        unchanged();
        auto controller=widget.findChild<igQtAnimationVcrController*>(); Check(controller!=nullptr,"VCR missing");
        // Regression (2026-09-28, commit: 待提交): cache final independent outputs
        // for both playback modes; revisiting a frame must not rerun extraction
        // or mutate the source, and changing capacity must not target the output's
        // time-frame metadata instead of the animation cache.
        Control<QComboBox>(widget,"comboBox_AnimationCacheNum")->setCurrentIndex(2);
        DataObject::Pointer cachedFirst;
        // Verify rendering as well as geometry: captured scalar coloring must
        // differ from a solid-color rendering of the exact same frame. Include
        // interpolation so it cannot silently reset the component to magnitude.
        auto verifyColoring=[&](const char* label) {
            auto display=DynamicCast<DrawObject>(scene->GetCurrentModel()->GetDataObject());
            Check(display && display->HasSubDataObject(),"missing display blocks");
            auto child=DynamicCast<DrawObject>(display->SubDataObjectIteratorBegin()->second);
            Check(child && child->IsUseColor(),"animation extraction did not enable scalar coloring");
            Check(child->GetAttributeDimension()==0,"animation lost display component");
            Check(child->GetColorMapper()==source->GetColorMapper(),"output lost source color mapper");
            scene->MakeCurrent(); glBindFramebuffer(GL_FRAMEBUFFER,0); scene->Draw(); glFinish();
            auto colored=scene->CaptureScreen(0,0,160,120,GLFramebuffer::Type::RGBA,true);
            Check(colored.size()==160*120*4,"color frame capture failed");
            QImage image(colored.data(),160,120,QImage::Format_RGBA8888);
            Check(image.save(QCoreApplication::applicationDirPath()+"/animation-color-"+label+".png"),"color screenshot failed");
            display->ViewCloudPicture(scene,-1);
            glBindFramebuffer(GL_FRAMEBUFFER,0); scene->Draw(); glFinish();
            auto solid=scene->CaptureScreen(0,0,160,120,GLFramebuffer::Type::RGBA,true);
            Check(solid.size()==colored.size(),"solid frame capture failed");
            int changed=0;
            for (size_t p=0;p<colored.size();p+=4)
                if (colored[p]!=solid[p] || colored[p+1]!=solid[p+1] || colored[p+2]!=solid[p+2]) ++changed;
            Check(changed>20,"scalar rendering is indistinguishable from solid color");
            display->ViewCloudPicture(scene,display->GetAttributeSet()->GetAttributeIndex("density"),0);
            Check(glGetError()==GL_NO_ERROR,"color rendering OpenGL error");
        };
        for (int i=0;i<2;++i) { controller->updateCurrentKeyframe(i);
            auto display=scene->GetCurrentModel()->GetDataObject();
            if (i==0) cachedFirst=display;
            Check(display!=source && display->HasSubDataObject(),"animation did not present extraction output");
            auto mesh=DynamicCast<UnstructuredMesh>(display->SubDataObjectIteratorBegin()->second);
            Check(mesh && mesh->GetNumberOfCells()>0,"animation volume empty");
            // Regression (2026-09-28, commit: see file header): with only one density
            // array, pre-setting its index made ViewCloudPicture return early.
            // Every newly extracted frame must enable coloring on the actual
            // drawable child, with the Filter's selected component (not -1).
            Check(mesh->IsUseColor(),"animation extraction did not enable scalar coloring");
            Check(mesh->GetAttributeIndex()==mesh->GetAttributeSet()->GetAttributeIndex("density") &&
                  mesh->GetAttributeDimension()==0,"animation lost display field/component");
            auto field=mesh->GetAttributeSet()->GetAttribute("density");
            for (IGsize p=0;p<mesh->GetNumberOfPoints();++p) {
                Check(field.pointer->GetValue(p)>=.25-1e-5 && field.pointer->GetValue(p)<=.75+1e-5,"wrong animation iso range");
                const double z=mesh->GetPoint(p)[2];
                Check(z>=(i ? .625 : .25)-1e-5 && z<=(i ? .875 : .75)+1e-5,"frame geometry did not update");
            }
            verifyColoring(i ? "second" : "first");
        }
        controller->updateCurrentKeyframe(0);
        Check(scene->GetCurrentModel()->GetDataObject()==cachedFirst,"final output cache missed");
        unchanged();
        checkLegacy();
        Control<QComboBox>(widget,"comboBox_AnimationCacheNum")->setCurrentIndex(0);
        checkLegacy();
        controller->updateCurrentKeyframe(0);
        Check(scene->GetCurrentModel()->GetDataObject()!=cachedFirst,"zero capacity retained final output");
        Control<QComboBox>(widget,"comboBox_AnimationCacheNum")->setCurrentIndex(2);
        Check(QMetaObject::invokeMethod(&widget,"playAnimation_interpolate",Qt::DirectConnection,
                                       Q_ARG(int,0),Q_ARG(float,.5f)),"interpolation slot failed");
        verifyColoring("interpolated");
        auto interpolated=scene->GetCurrentModel()->GetDataObject();
        Check(QMetaObject::invokeMethod(&widget,"playAnimation_interpolate",Qt::DirectConnection,
                                       Q_ARG(int,0),Q_ARG(float,.5f)),"interpolation replay failed");
        Check(scene->GetCurrentModel()->GetDataObject()==interpolated,"interpolation output cache missed");
        unchanged();
        // Regression (2026-09-28, fix: 待提交): each frame reapplied the Filter's
        // default selection. Use the EXISTING expand-only mode instead of adding
        // a manual-fixed mode to the shared scalar panel. Its three modes and
        // explicit per-frame selection must remain available (fix: 待提交).
        Check(Control<QComboBox>(scalar,"comboBox_RangeMode")->count()==3,"unexpected public range mode added");
        Check(Control<QComboBox>(scalar,"comboBox_RangeMode")->currentIndex()==1,"animation did not initialize expand-only mode");
        auto mapper=scene->GetCurrentModel()->GetDataObject()->GetColorMapper();
        mapper->InitColorBarWithGrayScaleType();
        auto palette=mapper->GetColorBar();
        auto checkColorState=[&] {
            scene->MakeCurrent(); glBindFramebuffer(GL_FRAMEBUFFER,0); scene->Draw(); glFinish();
            auto output=DynamicCast<DrawObject>(scene->GetCurrentModel()->GetDataObject());
            Check(output->IsUseColor(),"model tree refresh disabled coloring");
            Check(output->GetColorMapper()==mapper && mapper->GetColorBar()==palette,"frame reset palette");
            Check(Control<QComboBox>(scalar,"comboBox_RangeMode")->currentIndex()==1,"expand-only mode lost on frame change");
            Check(modelRow->getCurrentChild()!=nullptr,"tree lost selected field");
        };
        Check(widget.renderAnimationOutputFrame(0),"expand-only first frame"); checkColorState();
        Check(widget.renderAnimationOutputFrame(1),"expand-only next frame"); checkColorState();
        Check(widget.renderAnimationOutputFrame(0,true),"expand-only cached export"); checkColorState();
        Control<QComboBox>(widget,"comboBox_AnimationCacheNum")->setCurrentIndex(0);
        Check(widget.renderAnimationOutputFrame(1),"expand-only uncached output"); checkColorState();
        Check(QMetaObject::invokeMethod(&widget,"playAnimation_interpolate",Qt::DirectConnection,
                                       Q_ARG(int,0),Q_ARG(float,.5f)),"expand-only interpolation"); checkColorState();
        auto output=DynamicCast<DrawObject>(scene->GetCurrentModel()->GetDataObject());
        output->ViewCloudPicture(scene,output->GetAttributeSet()->GetAttributeIndex("temperature"),-1);
        scalar.showScalarView();
        Check(widget.renderAnimationOutputFrame(0),"manual field next frame");
        output=DynamicCast<DrawObject>(scene->GetCurrentModel()->GetDataObject());
        Check(output->GetAttributeIndex()==output->GetAttributeSet()->GetAttributeIndex("temperature") &&
              output->GetAttributeDimension()==-1,"filter reset chosen field/component");
        output->ViewCloudPicture(scene,-1);
        Check(widget.renderAnimationOutputFrame(1),"solid-color next frame");
        Check(!DynamicCast<DrawObject>(scene->GetCurrentModel()->GetDataObject())->IsUseColor(),"filter reenabled scalar coloring");
        output=DynamicCast<DrawObject>(scene->GetCurrentModel()->GetDataObject());
        output->ViewCloudPicture(scene,output->GetAttributeSet()->GetAttributeIndex("density"),0);
        scalar.showScalarView();
        Control<QComboBox>(scalar,"comboBox_RangeMode")->setCurrentIndex(0);
        Check(widget.renderAnimationOutputFrame(0),"automatic range next frame");
        Check(!mapper->GetStable() && std::abs(mapper->GetRange()[0]-.25)<1e-5 &&
              std::abs(mapper->GetRange()[1]-.75)<1e-5,"automatic range did not resume");
        Check(Control<QComboBox>(scalar,"comboBox_RangeMode")->currentIndex()==0,"explicit per-frame choice overridden");
        Control<QComboBox>(widget,"comboBox_AnimationCacheNum")->setCurrentIndex(2);
        // Invalidation must cover saved parameters and an empty pipeline too.
        list->setCurrentRow(1); Control<QPushButton>(widget,"btnAnimationFilterParameters")->click();
        QApplication::processEvents();
        dialog=Control<igQtFilterDialogDockWidget>(widget,"animationFilterParameters");
        Control<QLineEdit>(*dialog,"animationParam_lowerValue")->setText("0.4"); dialog->apply(); dialog->close();
        Check(QMetaObject::invokeMethod(&widget,"playAnimation_interpolate",Qt::DirectConnection,
                                       Q_ARG(int,0),Q_ARG(float,.5f)),"changed interpolation failed");
        Check(scene->GetCurrentModel()->GetDataObject()!=interpolated,"parameter change used stale output");
        auto changed=scene->GetCurrentModel()->GetDataObject()->SubDataObjectIteratorBegin()->second;
        auto changedDensity=changed->GetAttributeSet()->GetAttribute("density").pointer;
        for (IGsize i=0;i<changedDensity->GetNumberOfValues();++i)
            Check(changedDensity->GetValue(i)>=.4-1e-5,"old extraction threshold was cached");
        Control<QPushButton>(widget,"btnAnimationFilterClear")->click();
        Check(widget.renderAnimationOutputFrame(0),"empty pipeline output failed");
        auto empty=scene->GetCurrentModel()->GetDataObject();
        Check(empty->SubDataObjectIteratorBegin()->second->GetAttributeSet()->GetAttribute("density").attachmentType==IG_CELL,
              "empty pipeline returned converted cached data");
        widget.initAnimationComponents();
        Check(widget.renderAnimationOutputFrame(1),"empty pipeline second output failed");
        auto& lockedRange=scene->GetCurrentModel()->GetDataObject()->GetAttributeSet()->GetAttribute("density");
        lockedRange.rangeLocked=true; lockedRange.rangeLockedDimension=0;
        lockedRange.GetDataRange()->SetElement(1,{-1.,5.});
        // Simulate unavailable disk data: export must reuse the displayed final
        // output cache, not enter a separate file-loading path.
        fs::rename(dir/"frame0.vtu",dir/"frame0.hidden");
        const bool exportHit=widget.renderAnimationOutputFrame(0,true);
        fs::rename(dir/"frame0.hidden",dir/"frame0.vtu");
        Check(exportHit && scene->GetCurrentModel()->GetDataObject()==empty,"export did not share final-output cache");
        Check(empty->GetAttributeSet()->GetAttribute("density").rangeLocked,"cache hit lost range lock");
        auto& expanding=empty->GetAttributeSet()->GetAttribute("density");
        expanding.rangeMode=AttributeSet::RangeMode::ExpandOnly;
        expanding.runningRangeValid=true; expanding.runningMin=0; expanding.runningMax=2;
        Check(widget.renderAnimationOutputFrame(1),"expanding range output failed");
        Check(scene->GetCurrentModel()->GetDataObject()->GetAttributeSet()->GetAttribute("density").runningMax==4,
              "cached frame did not expand color range");
        Check(widget.renderAnimationOutputFrame(0),"expanding range replay failed");
        Check(empty->GetAttributeSet()->GetAttribute("density").runningMax==4,"cached frame shrank expanding range");
        Check(std::abs(mapper->GetRange()[0])<1e-5 && std::abs(mapper->GetRange()[1]-4)<1e-5,
              "scalar panel shrank the accumulated range on cached replay");
        Control<QRadioButton>(widget,"rbtnInterpolateTimeMode")->setChecked(true);
        Control<QLineEdit>(widget,"lineEditKeyframeNum")->setText("3");
        Control<QPushButton>(widget,"btnApplyAnimationOperation")->click();
        Check(widget.animationOutputFrameCount()==3,"export still uses original frame count");
        Check(widget.renderAnimationOutputFrame(1,true),"interpolated export output failed");
        auto middle=scene->GetCurrentModel()->GetDataObject();
        auto middleDensity=middle->SubDataObjectIteratorBegin()->second->GetAttributeSet()->GetAttribute("density").pointer;
        Check(std::abs(middleDensity->GetValue(1)-3.)<1e-5,"export did not interpolate raw field");
        Check(std::abs(mapper->GetRange()[1]-4)<1e-5,"interpolation shrank accumulated range");
        controller->updateCurrentKeyframe(1);
        Check(scene->GetCurrentModel()->GetDataObject()==middle,"playback did not share export output cache");
        Control<QLineEdit>(widget,"lineEditKeyframeNum")->setText("5");
        Control<QPushButton>(widget,"btnApplyAnimationOperation")->click();
        Check(widget.renderAnimationOutputFrame(1),"new timeline output failed");
        auto quarter=scene->GetCurrentModel()->GetDataObject();
        Check(quarter!=middle && std::abs(quarter->SubDataObjectIteratorBegin()->second->GetAttributeSet()->GetAttribute("density").pointer->GetValue(1)-2.5)<1e-5,
              "timeline change returned stale middle frame");
        unchanged();
        auto secondSource=FileIO::ReadFile((dir/"data.pvd").generic_string());
        auto secondId=scene->AddModel(secondSource);
        scene->SetCurrentModel(static_cast<int>(secondId)); widget.initAnimationComponents();
        Check(widget.renderAnimationOutputFrame(0),"new source output failed");
        Check(scene->GetCurrentModel()->GetDataObject()!=empty && scene->GetCurrentModel()->GetDataObject()!=quarter,
              "source switch reused old cache");
        scene->SetCurrentModel(static_cast<int>(id)); widget.initAnimationComponents();
        Check(widget.renderAnimationOutputFrame(0),"returning source output failed");
        Check(scene->GetCurrentModel()->GetDataObject()!=empty,"returning source retained stale cache");
        scene->RemoveModel(secondId);
    }
    checkLegacy();
    Check(legacyFrames->GetTargetTimeFrameData(0)==legacyFirst,"legacy cache no longer hits after animation");
    legacyFirst.clear(); legacySecond.clear(); legacyField=nullptr;
    legacyFrames=nullptr;
    scene->RemoveModel(id); source=nullptr; scene->Finalize(); glfwDestroyWindow(window); glfwTerminate();
}
// Real-data acceptance (2026-09-28, commit: see file header): compressed VTU animation
// frames contain only Cell Density. Verify metadata-only configuration, actual
// point conversion and bounded IsoVolume output on first/middle/last samples.
void DatasetTest(const std::filesystem::path& directory) {
    DataObject::DeferDrawableConversionScope cpu;
    igQtAnimationFilterManager manager; QString error;
    Check(igQtRegisterBuiltinAnimationFilters(manager,&error),"register dataset filters");
    for (int frame : {1,50,100}) {
        const auto start=std::chrono::steady_clock::now();
        auto source=FileIO::ReadFile((directory/("density_iter_"+std::to_string(frame)+".vtu")).generic_string());
        Check(source!=nullptr,"dataset frame read failed");
        auto attrs=source->GetAttributeSet();
        Check(attrs && attrs->GetAttributeIndex("Density")>=0,"missing Density");
        const auto original=attrs->GetAttribute("Density").pointer;
        Check(attrs->GetAttribute("Density").attachmentType==IG_CELL,"expected cell Density");
        igQtAnimationDataInfo input, converted;
        Check(igQtDescribeAnimationData(source,input,error),"describe dataset");
        Check(manager.describeOutput("convertToPointData",input,{},converted,error),error.toStdString().c_str());
        igQtAnimationFilterParameterSchema schema;
        Check(manager.parameterSchema("isoVolume",converted,schema,error),error.toStdString().c_str());
        Check(schema[0].choices.contains("Density"),"inferred Density missing");
        Check(attrs->GetAttribute("Density").pointer==original && attrs->GetAttribute("Density").attachmentType==IG_CELL,"description mutated dataset");
        igQtAnimationFrameContext context; context.input=source;
        auto pointResult=manager.execute("convertToPointData",context,{});
        Check(pointResult.success,pointResult.error.toStdString().c_str());
        const auto pointField=source->GetAttributeSet()->GetAttribute("Density");
        Check(pointField.attachmentType==IG_POINT,"conversion did not produce Point Density");
        double minimum=1e300, maximum=-1e300;
        for (IGsize i=0;i<pointField.pointer->GetNumberOfValues();++i) {
            const double value=pointField.pointer->GetValue(i);
            Check(std::isfinite(value),"non-finite converted density");
            minimum=std::min(minimum,value); maximum=std::max(maximum,value);
        }
        const QVariantMap parameters{{"scalarName","Density"},{"scalarDimension",0},{"lowerValue",.2},{"upperValue",1.0}};
        auto result=manager.execute("isoVolume",context,parameters);
        Check(result.success,result.error.toStdString().c_str());
        auto output=DynamicCast<UnstructuredMesh>(result.output);
        Check(output && output->GetNumberOfCells()>0,"dataset produced empty iso volume");
        auto values=output->GetAttributeSet()->GetAttribute("Density").pointer;
        for (IGsize i=0;i<values->GetNumberOfValues();++i)
            Check(values->GetValue(i)>=.2-1e-5 && values->GetValue(i)<=1.0+1e-5,"dataset iso bounds violated");
        const auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
        std::cout<<"DATASET frame="<<frame<<" pointDensity=["<<minimum<<","<<maximum
                 <<"] isoPoints="<<output->GetNumberOfPoints()<<" isoCells="<<output->GetNumberOfCells()
                 <<" seconds="<<seconds<<std::endl;
    }
}
// Acceptance (2026-09-28, commit: 待提交): use the user's real PVD in the same
// widget/output path as playback and export; repeat samples to verify hits and
// ensure the old source cache remains empty, with the source still Cell Density.
void DatasetCacheTest(const std::filesystem::path& directory) {
    Check(std::filesystem::exists("Resources/Shaders/FullScreenTriangle.vert"),"run dataset-cache from the build directory");
    Check(glfwInit()!=0,"dataset GLFW init"); glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,4); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,6);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    auto window=glfwCreateWindow(320,240,"Dataset output cache",nullptr,nullptr);
    Check(window!=nullptr,"dataset OpenGL unavailable"); glfwMakeContextCurrent(window);
    auto scene=SceneManager::Instance()->NewScene(); scene->Initialize(); scene->Resize(320,240,1);
    scene->EnableFramePacing(false); scene->SetMakeCurrentFunctor([&] { glfwMakeContextCurrent(window); });
    auto source=FileIO::ReadFile((directory/"Result.pvd").generic_string()); Check(source!=nullptr,"read dataset PVD");
    auto id=scene->AddModel(source); scene->ResetCameraView(source->GetBoundingBox());
    {
        igQtAnimationWidget widget; widget.initAnimationComponents(); widget.setPreferredCacheNum(2);
        auto combo=Control<QComboBox>(widget,"comboBoxAnimationFilter");
        for (const char* name : {"convertToPointData","isoVolume"}) {
            combo->setCurrentIndex(combo->findData(name)); Control<QPushButton>(widget,"btnAnimationFilterAdd")->click();
        }
        Control<QListWidget>(widget,"listWidgetAnimationPipeline")->setCurrentRow(1);
        Control<QPushButton>(widget,"btnAnimationFilterParameters")->click(); QApplication::processEvents();
        auto dialog=Control<igQtFilterDialogDockWidget>(widget,"animationFilterParameters");
        Check(Control<QComboBox>(*dialog,"animationParam_scalarName")->currentText()=="Density","dataset field inference");
        Control<QLineEdit>(*dialog,"animationParam_lowerValue")->setText("0.2");
        Control<QLineEdit>(*dialog,"animationParam_upperValue")->setText("1.0"); dialog->apply(); dialog->close();
        for (int frame : {0,49,99}) {
            const auto start=std::chrono::steady_clock::now();
            Check(widget.renderAnimationOutputFrame(frame),"real dataset output failed");
            auto output=scene->GetCurrentModel()->GetDataObject();
            auto mesh=DynamicCast<UnstructuredMesh>(output->SubDataObjectIteratorBegin()->second);
            Check(mesh && mesh->GetNumberOfCells()>0 && mesh->IsUseColor(),"real output geometry/color missing");
            const auto hitStart=std::chrono::steady_clock::now();
            Check(widget.renderAnimationOutputFrame(frame,true),"real dataset export hit failed");
            const auto end=std::chrono::steady_clock::now();
            Check(scene->GetCurrentModel()->GetDataObject()==output,"real dataset final output not reused");
            Check(source->PeekTimeFrames()->GetCurrentCacheCount()==0,"real dataset source cache active");
            Check(source->SubDataObjectIteratorBegin()->second->GetAttributeSet()->GetAttribute("Density").attachmentType==IG_CELL,
                  "real source was converted in place");
            std::cout<<"DATASET_CACHE frame="<<frame+1<<" isoCells="<<mesh->GetNumberOfCells()
                     <<" computeSeconds="<<std::chrono::duration<double>(hitStart-start).count()
                     <<" hitSeconds="<<std::chrono::duration<double>(end-hitStart).count()<<std::endl;
        }
        scene->MakeCurrent(); glBindFramebuffer(GL_FRAMEBUFFER,0); scene->Draw(); glFinish();
        auto pixels=scene->CaptureScreen(0,0,320,240,GLFramebuffer::Type::RGBA,true);
        Check(pixels.size()==320*240*4,"dataset screenshot");
        QImage image(pixels.data(),320,240,QImage::Format_RGBA8888);
        image.save(QCoreApplication::applicationDirPath()+"/output-cache-density.png");
        Check(glGetError()==GL_NO_ERROR,"dataset rendering error");
    }
    scene->RemoveModel(id); source=nullptr; scene->Finalize(); glfwDestroyWindow(window); glfwTerminate();
}

// Regression investigation (2026-09-29, fix: 待提交): opening the user's
// 2.32 GB sukong VTU crashes. Log completed stages to distinguish reading,
// static-model animation initialization and the first render.
void SingleFileTest(const std::filesystem::path& path) {
    Check(glfwInit()!=0,"file GLFW init"); glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,4); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,6);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    auto window=glfwCreateWindow(320,240,"Single file regression",nullptr,nullptr);
    Check(window!=nullptr,"file OpenGL unavailable"); glfwMakeContextCurrent(window);
    auto scene=SceneManager::Instance()->NewScene(); scene->Initialize(); scene->Resize(320,240,1);
    scene->SetMakeCurrentFunctor([&] { glfwMakeContextCurrent(window); });
    std::cout<<"FILE_STAGE reading "<<path.generic_string()<<std::endl;
    auto source=FileIO::ReadFile(path.generic_string()); Check(source!=nullptr,"single file read failed");
    auto mesh=DynamicCast<PointSet>(source); Check(mesh!=nullptr,"single file mesh missing");
    std::cout<<"FILE_STAGE read points="<<mesh->GetNumberOfPoints()<<" cells="<<(mesh->GetCellArray() ? mesh->GetCellArray()->GetNumberOfCells() : 0)
             <<" fields="<<source->GetAttributeSet()->GetNumberOfAttributes()<<std::endl;
    const auto id=scene->AddModel(source); scene->ResetCameraView(source->GetBoundingBox());
    { igQtAnimationWidget widget; widget.initAnimationComponents();
      std::cout<<"FILE_STAGE animation initialized"<<std::endl;
      scene->MakeCurrent(); scene->Draw(); glFinish();
      Check(glGetError()==GL_NO_ERROR,"single file draw failed");
      std::cout<<"FILE_STAGE rendered"<<std::endl;
    }
    scene->RemoveModel(id); mesh=nullptr; source=nullptr;
    scene->Finalize(); glfwDestroyWindow(window); glfwTerminate();
}
void SingleFileUiTest(const std::filesystem::path& path) {
    QSurfaceFormat format; format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(4,6); format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(32); format.setStencilBufferSize(8); format.setSamples(1);
    QSurfaceFormat::setDefaultFormat(format);
    std::cout<<"UI_STAGE constructing"<<std::endl;
    igQtMainWindow window; window.setAttribute(Qt::WA_DontShowOnScreen);
    window.resize(1000,700); window.show(); QApplication::processEvents();
    std::cout<<"UI_STAGE opening"<<std::endl;
    window.initArgs({"file-regression","--filepath",QString::fromStdString(path.generic_string())});
    std::cout<<"UI_STAGE opened"<<std::endl;
    QEventLoop loop; QTimer::singleShot(2000,&loop,&QEventLoop::quit); loop.exec();
    auto scene=SceneManager::Instance()->GetCurrentScene();
    Check(scene && scene->GetCurrentModel(),"UI model missing");
    auto source=scene->GetCurrentModel()->GetDataObject();
    Check(source && DynamicCast<PointSet>(source),"UI data missing");
    std::cout<<"UI_STAGE rendered points="<<DynamicCast<PointSet>(source)->GetNumberOfPoints()<<std::endl;
}
int main(int argc,char** argv) {
    Q_INIT_RESOURCE(iGameQtMainWindow);
    QApplication app(argc,argv); Log::Init();
    try {
        if (argc>2 && std::string(argv[1])=="file-ui") SingleFileUiTest(argv[2]);
        else if (argc>2 && std::string(argv[1])=="file") SingleFileTest(argv[2]);
        else if (argc>2 && std::string(argv[1])=="dataset-cache") DatasetCacheTest(argv[2]);
        else if (argc>2 && std::string(argv[1])=="dataset") DatasetTest(argv[2]);
        else if (argc>1 && std::string(argv[1])=="dialog") DialogTest(); else Descriptions();
        std::cout<<"PASS field descriptions and execution\n"; return 0;
    }
    catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<'\n'; return 1; }
}
