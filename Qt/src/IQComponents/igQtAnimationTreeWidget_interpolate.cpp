//
// Created by m_ky on 2024/3/19.
//

#include <IQComponents/igQtAnimationTreeWidget_interpolate.h>
#include <QDebug>
igQtAnimationTreeWidget_interpolate::igQtAnimationTreeWidget_interpolate(QWidget *parent) : igQtAnimationTreeWidget(parent) {
//    setHeaderLabels({"插值时间序列", "值"});
}


void igQtAnimationTreeWidget_interpolate::updateInterpolateSequence(int num) {
    if (timeSequence.size() < 2 || num < 2) return;

    interpolate_timeSequence.clear();
    interpolate_timeSequence.reserve(num);
    keyframe_sum = num;
    current_Keyframe_index = 0;
    float frameStep = (endTime- startTime) / (float)(keyframe_sum - 1);

    std::vector<std::pair<int, float>> interpolate_sequence;
    int start_keyframe_idx = 0;
    auto it = timeSequence.begin() + 1;
    while(it != timeSequence.end() && startTime > *it)
    {
        it ++;
        start_keyframe_idx ++;
    }
    interpolate_sequence.reserve(keyframe_sum);
    interpolate_timeSequence.reserve(keyframe_sum);
    for(int i = 0; i < keyframe_sum; i ++)
    {
        float t = startTime + frameStep * (float)i;

        // 查找 t 所在的区间 [it-1, it]
        while(it != timeSequence.end() && t > *it)
        {
            it ++;
            start_keyframe_idx ++;
        }

        // ================== 【修复重点】 ==================
        // 如果迭代器跑到了 end()，说明 t 可能因为浮点误差稍微超过了 endTime
        // 或者 t 本身就比序列中最大的时间还要大。
        // 我们强制回退一格，使用最后一段区间进行计算。
        if (it == timeSequence.end()) {
            it--;
            start_keyframe_idx--;
        }
        // =================================================

        // 现在 it 绝对不是 end()，可以安全解引用
        float time_prev = *(it - 1);
        float time_curr = *it;

        // 防止分母为 0 (虽然在这个逻辑下不太可能，但为了健壮性)
        float ratio = 0.0f;
        if (time_curr - time_prev > 1e-6) {
            ratio = (t - time_prev) / (time_curr - time_prev);
        } else {
            // 如果两个关键帧时间重合，比例设为1或0均可，防止除0崩溃
            ratio = 1.0f;
        }

        interpolate_sequence.emplace_back(start_keyframe_idx, ratio);
        interpolate_timeSequence.push_back(t);
    }
    this->topLevelItem(2)->setText(1, QString("%1").arg(keyframe_sum));
    removeAllChildItem(this->topLevelItem(3));

    for(int i = 0; i < keyframe_sum; i ++)
    {
        auto *item = new QTreeWidgetItem();
        item->setText(0, QString("%1").arg(i + 1));
        item->setText(1, QString::asprintf("%.10f", interpolate_timeSequence[i]));
        this->topLevelItem(3)->addChild(item);
    }
    this->topLevelItem(3)->setText(1, QString("[%1, %2]").arg(startTime).arg(endTime));
    Q_EMIT updateVcrControllerInterpolateData(interpolate_sequence);
    Q_EMIT updateComponentsKeyframeSum(num);
}

void igQtAnimationTreeWidget_interpolate::initAnimationTreeWidget(std::vector<float> &timeSeq) {
    current_Keyframe_index = 0;
    timeSequence = timeSeq;
    startTime = *timeSeq.begin();
    endTime = *(timeSeq.end() - 1);
    if(timeSeq.size() > 1){
        updateInterpolateSequence(static_cast<int>(timeSeq.size()));
        updateData();
    }
}

void igQtAnimationTreeWidget_interpolate::updateData() {
    this->topLevelItem(0)->setText(1, QString("%1").arg(current_Keyframe_index + 1));
    this->topLevelItem(1)->setText(1, QString::asprintf("%.10f", interpolate_timeSequence[current_Keyframe_index]));
}

bool igQtAnimationTreeWidget_interpolate::updateInterpolateData(float _start, float _end, int keyframeNum) {
    if (timeSequence.size() < 2 || keyframeNum < 2 || _start >= _end) return false;
    if (_start < timeSequence.front() || _end > timeSequence.back()) return false;

    if((startTime != _start || endTime != _end || keyframe_sum != keyframeNum)) {
        startTime = _start;
        endTime = _end;
        updateInterpolateSequence(keyframeNum);
    }
    return true;
}

void igQtAnimationTreeWidget_interpolate::updateCurrentKeyframe(int idx) {
    if(idx >= keyframe_sum) return;
    current_Keyframe_index = idx;
    this->updateData();
}




