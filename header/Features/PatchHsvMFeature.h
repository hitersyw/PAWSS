#ifndef PATCH_HSVM_FEATURE_H
#define PATCH_HSVM_FEATURE_H
#include "Features/PatchFeature.h"
#include "Features/HsvFeature.h"
#include "Features/MotFeature.h"
#include "segModel.h"
#include "Config.h"

class PatchHsvMFeature : public PatchFeature
{
public:
    PatchHsvMFeature(const Config& conf);
    void UpdateWeightModel(const Sample &s);
    void setPrevImg(ImageRep &img);

private:
    void UpdateFeatureVector(const Sample& s);
    void PrepEval(const multiSample& samples);

    int mHsvBinNum;
    int mMotBinNum;
    int mBinNum;
    Config::kernelType mKernelType;

    HsvFeature mHsvFeature;
    MotFeature mMotFeature;

    segModel mWeightModel;

    std::vector<cv::Mat> mIntegs;
    cv::Mat mWeightInteg;

    double mColorWeight;
    double mMotWeight;

    bool mPatchWeightInitialized;
    cv::Mat mPrevImg;

};

#endif
