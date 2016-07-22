#ifndef PATCH_MOT_FEATURE_H
#define PATCH_MOT_FEATURE_H
#include "Features/PatchFeature.h"
#include "Features/MotFeature.h"
#include "Config.h"

class PatchMotFeature : public PatchFeature
{
public:
    PatchMotFeature(const Config& conf);
    void UpdateWeightModel(const Sample& s);
    void setPrevImg(ImageRep& img);

private:
    void PrepEval(const multiSample& samples);
    void UpdateFeatureVector(const Sample& s);

    int mBinNum;
    Config::kernelType mKernelType;

    MotFeature mMotFeature;
    std::vector<cv::Mat> mIntegs;
    cv::Mat mWeightInteg;

    cv::Mat mPrevImg;
    cv::Mat mCurrImg;

};

#endif
