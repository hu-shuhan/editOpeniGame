// Regression (2026-09-28, commit: 待提交): source-frame caching still executed
// filters on every visit, and interpolation bypassed it. Cache only successful
// final outputs, including an empty pipeline, and never retain intermediate data.
// Exercise producer/filter counts, LRU order, shrinking, disabling, invalidation,
// failure retries and distinct interpolation weights to prevent stale results.
#include <IQCore/igQtAnimationOutputCache.h>
#include <IQCore/igQtAnimationPipeline.h>
#include <IQCore/igQtAnimationFilterManager.h>
#include <iostream>
#include <stdexcept>

void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
int main() {
    try {
        igQtAnimationOutputCache cache;
        igQtAnimationFilterManager manager;
        int loads=0, executions=0;
        igQtAnimationFilterDescriptor filter;
        filter.id="count"; filter.displayName="Count";
        filter.execute=[&](const igQtAnimationFrameContext&,const QVariantMap&) {
            ++executions; igQtAnimationFilterResult result;
            result.success=true; result.output=iGame::DataObject::New();
            result.displayAttribute="density"; result.displayDimension=2;
            return result;
        };
        Require(manager.registerFilter(filter),"registration");
        igQtAnimationPipelineSteps pipeline{{"count",{}}};
        auto produce=[&] {
            ++loads; igQtAnimationFrameContext context; context.input=iGame::DataObject::New();
            igQtAnimationFilterResult result; QString error;
            Require(igQtExecuteAnimationPipeline(manager,pipeline,context,result,&error),"execute");
            return result;
        };
        cache.setCapacity(2);
        auto first=cache.resolve({0},produce); cache.resolve({1},produce);
        auto hit=cache.resolve({0},produce);
        Require(loads==2 && executions==2 && hit.output==first.output,"cache reran IO or pipeline");
        Require(hit.displayAttribute=="density" && hit.displayDimension==2,"lost display metadata");
        cache.resolve({2},produce); cache.resolve({0},produce);
        Require(loads==3,"LRU evicted recently accessed frame");
        cache.resolve({1},produce); Require(loads==4 && cache.size()==2,"LRU capacity");
        cache.setCapacity(1); Require(cache.size()==1,"shrinking is not immediate");
        cache.setCapacity(0); Require(cache.size()==0,"disable did not clear");
        cache.resolve({0},produce); cache.resolve({0},produce);
        Require(loads==6 && cache.size()==0,"zero capacity cached output");
        cache.setCapacity(3); cache.resolve({0,true,.25f},produce);
        cache.resolve({0,true,.75f},produce); cache.resolve({0,true,.25f},produce);
        Require(loads==8,"interpolation key collision or missed hit");
        cache.resolve({0},produce); Require(loads==9,"snap/interpolation key collision");
        cache.clear(); cache.resolve({0},produce); Require(loads==10,"invalidation ignored");
        pipeline.clear(); cache.clear();
        auto empty=cache.resolve({0},produce); auto emptyHit=cache.resolve({0},produce);
        Require(loads==11 && executions==10 && empty.output==emptyHit.output,"empty pipeline bypassed output cache");
        cache.clear(); int failures=0;
        auto fail=[&] { ++failures; return igQtAnimationFilterResult{}; };
        cache.resolve({0},fail); cache.resolve({0},fail);
        Require(failures==2 && cache.size()==0,"failure was cached");
        std::cout<<"PASS final-output cache\n";
        return 0;
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
