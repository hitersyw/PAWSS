#include <iostream>
#include <opencv/highgui.h>
#include <opencv2/opencv.hpp>
#include <PAWSS/Config.h>
#include <PAWSS/Rect.h>
#include <PAWSS/mUtils.h>
#include <PAWSS/Tracker.h>
#include <PAWSS/macros.h>

static const cv::Scalar COLOR_CYAN = CV_RGB(0, 255, 255);
static const cv::Scalar COLOR_RED = CV_RGB(255, 0, 0);
static const cv::Scalar COLOR_GREEN = CV_RGB(0, 255, 0);
static const cv::Scalar COLOR_BLUE = CV_RGB(0, 0, 255);
static const cv::Scalar COLOR_YELLOW = CV_RGB(255, 255, 0);
static const cv::Scalar COLOR_MAGENTA = CV_RGB(255, 0, 255);
static const cv::Scalar COLOR_WHITE = CV_RGB(255, 255, 255);
static const cv::Scalar COLOR_BLACK = CV_RGB(0, 0, 0);

bool mouse_drawing = false;
FloatRect roi;
bool return_flag = false;

// choose rectangle roi on the frame image using right click and drag
void chooseRoiCallBack(int event, int x, int y, int flags, void* params)
{
    PAWSS_UNUSED(flags);
    PAWSS_UNUSED(params);
    if(event == cv::EVENT_LBUTTONDOWN)
    {
        mouse_drawing = true;
        roi = FloatRect(x, y, 0, 0);
    }
    else if (event == cv::EVENT_MOUSEMOVE)
    {
        if(mouse_drawing)
            roi.Set(roi.XMin(), roi.YMin(), x - roi.XMin(), y - roi.YMin());
    }
    else if (event == cv::EVENT_LBUTTONUP)
    {
        mouse_drawing = false;
        roi.Set(roi.XMin(), roi.YMin(), x - roi.XMin(), y - roi.YMin());
    }
}

void drawOnImage(cv::Mat& img)
{
    rectangle(img, roi, COLOR_GREEN);
    cv::putText(img, "["+std::to_string(int(roi.XMin()))+","+std::to_string(int(roi.YMin()))+","
                             +std::to_string(int(roi.Width()))+","+std::to_string(int(roi.Height()))+"]",
                             cv::Point(10, 20), cv::FONT_HERSHEY_PLAIN, 0.8, COLOR_GREEN);

}

bool trackSequence(Config& conf, const size_t seqIdx)
{
    bool deinterlace_flag = false;
    bool paused = false;

    int startFrame, endFrame;
    float scaleW, scaleH;
    FloatRect initBB;
    bool gt_flag;
    std::string framesFilePath, gtFilePath, imgFormat, imgResultFormat;
    std::vector<FloatRect> gtBBs, rBBs;
    char imgPath[256];

    cv::VideoCapture cap;
    if(conf.mUseCamera)
    {
        cap = cv::VideoCapture(0);
        if(!cap.open(0))
        {
            std::cout <<"error: could not start camera capture" << std::endl;
            return false;
        }
        startFrame = 0;
        endFrame = INT_MAX;
        gt_flag = false;
        conf.mSeqName = "cap";
    }
    else
    {
        // parse frames file
        assert(seqIdx < conf.mSeqNames.size());
        conf.mSeqName = conf.mSeqNames[seqIdx];
        framesFilePath = conf.mSeqBasePath + "/" + conf.mSeqName + "/" + conf.mSeqName+"_frames.txt";
        // read startFrame and endFrame from frames file
        if(!readFramesFile(framesFilePath, startFrame, endFrame))
            return false;
        gtFilePath = conf.mSeqBasePath+"/"+conf.mSeqName+"/"+conf.mSeqName+"_gt.txt";
        // read ground truth bounding box from gt file
        gt_flag = readGtFile(gtFilePath, gtBBs);
        imgFormat = conf.mSeqBasePath+"/"+conf.mSeqName+"/img/"+"%04d.jpg";
    }
    if(conf.mSaveFrame)
    {
        std::string imgResultPath = conf.mSeqBasePath+"/"+conf.mSeqName+"/result/";
        mkdir(imgResultPath);
        std::cout << imgResultPath << std::endl;
        imgResultFormat = imgResultPath+"r_%04d.png";
    }


    if(!gt_flag)
    {
        cv::Mat tmp, img;

        if(conf.mUseCamera)
        {
            std::cout << "use web camera: " << std::endl;
            std::cout << "manually choose roi: " << std::endl;
            std::cout << "RIGHT click and drag roi on the image"<< std::endl;
            std::cout << "Press 'i' to display roi information." << std::endl;
            std::cout << "Press 'r' to reset roi." << std::endl;
            std::cout << "Press 'q' or 'ESC' to finish." << std::endl;
            std::cout << std::endl;
            while(true)
            {
                cap >> tmp;
                cv::setMouseCallback("choose roi", chooseRoiCallBack, NULL);
                img = tmp.clone();
                drawOnImage(img);
                cv::imshow("choose roi", img);
                int k = cv::waitKey(1);
                if(k == 'i')
                    std::cout<<"ROI: "<< roi <<std::endl;
                if(k == 'r')
                {
                    mouse_drawing = false;
                    roi = FloatRect(0, 0, 0, 0);
                }
                if(k == 27 || k == 'q')
                {
                    cv::destroyWindow("choose roi");
                    break;
                }
            }
        }
        else
        {
            sprintf(imgPath, imgFormat.c_str(), startFrame);
            tmp = cv::imread(imgPath, cv::IMREAD_COLOR);
            if(tmp.empty())
            {
                std::cout << "error: could not read frame: " << imgPath << std::endl;
                return false;
            }
            if(!gt_flag) {
                while(true) {
                    cv::setMouseCallback("choose roi", chooseRoiCallBack, NULL);
                    img = tmp.clone();
                    drawOnImage(img);
                    cv::imshow("choose roi", img);
                    int k = cv::waitKey(1);
                    if(k == 'i')
                        std::cout<<"ROI: "<< roi <<std::endl;
                    if(k == 'r')
                    {
                        mouse_drawing = false;
                        roi = FloatRect(0, 0, 0, 0);
                    }
                    if(k == 27 || k == 'q')
                    {
                        cv::destroyWindow("choose roi");
                        break;
                    }
                }
            }
        }
    }

    if(gt_flag) {
        initBB = gtBBs[0]; }
    else {
        initBB = roi; }


    // scale the bounding box
    if(conf.mUseCamera)
    {
        scaleW = 0.3;
        scaleH = 0.3;
    }
    else
    {
        scaleFrame(initBB.Width(), initBB.Height(), scaleW, scaleH);
    }
    initBB = FloatRect(initBB.XMin()*scaleW, initBB.YMin()*scaleH, initBB.Width()*scaleW, initBB.Height()*scaleH);
    conf.mSearchRadius = std::round((initBB.Width()+initBB.Height())/4);

    // declare the tracker
    Tracker tracker(conf);
    srand(conf.mSeed);
    if(!conf.mQuietMode)
    {
        cv::namedWindow("result");
    }

    rBBs.clear();
    int frameInd = startFrame;
    while(frameInd <= endFrame && !return_flag)
    {
        cv::Mat frame, frameOrig, result;

        if(conf.mUseCamera)
        {
            cap >> frameOrig;
        }
        else
        {
            sprintf(imgPath, imgFormat.c_str(), frameInd);
            frameOrig = cv::imread(imgPath, cv::IMREAD_COLOR);
            if(frameOrig.empty())
            {
                std::cout << "error: could not read frame: " << imgPath << std::endl;
                if(gt_flag)
                    return false;
                else
                    continue;
            }
        }

        // deinterlace the frame or not
        if(!deinterlace_flag){
            deinterlace(frameOrig); }
        // scale the frame
        cv::resize(frameOrig, frame, cv::Size(std::round(frameOrig.cols*scaleW), std::round(frameOrig.rows*scaleH)));

        if(frameInd == startFrame)
        {
            tracker.Initialise(frame, initBB);
            FloatRect r(initBB.XMin()/scaleW, initBB.YMin()/scaleH, initBB.Width()/scaleW, initBB.Height()/scaleH);
            if(!conf.mUseCamera)
            {
                // store the bounding box
                rBBs.push_back(r);
            }
            //  save the result image or not
            if(conf.mSaveFrame)
            {
                sprintf(imgPath, imgResultFormat.c_str(), frameInd);
                rectangle(frameOrig, r, COLOR_CYAN, 3);
                cv::imwrite(imgPath, frameOrig);
            }
        }
        if(tracker.isInitialised())
        {
            tracker.Track(frame);

            if(frameInd != startFrame)
            {
                const FloatRect& bb = tracker.getBB();
                FloatRect r(bb.XMin()/scaleW, bb.YMin()/scaleH, bb.Width()/scaleW, bb.Height()/scaleH);
                if(!conf.mUseCamera)
                {
                    // store the bounding box
                    rBBs.push_back(r);
                }
                //  save the result image or not
                if(conf.mSaveFrame)
                {
                    sprintf(imgPath, imgResultFormat.c_str(), frameInd);
                    rectangle(frameOrig, r, COLOR_CYAN, 3);
                    cv::imwrite(imgPath, frameOrig);
                }
            }

            if(!conf.mQuietMode && conf.mDebugMode){
                tracker.Debug(frame);}
        }

        // show the tracking result or not
        if(!conf.mQuietMode)
        {
            frame.copyTo(result);
            rectangle(result, tracker.getBB(), COLOR_CYAN, 3);
            cv::putText(result, std::to_string(frameInd)+'/'+std::to_string(endFrame), cv::Point(10, 20), cv::FONT_HERSHEY_COMPLEX, 0.4, CV_RGB(0, 255, 0));
            cv::putText(result, std::to_string(tracker.getScale()), cv::Point(10, 40), cv::FONT_HERSHEY_COMPLEX, 0.4, CV_RGB(0, 255, 0));
            cv::imshow("result", result);
            int key = cv::waitKey(paused ? 0 : 1);
            if (key != -1)
            {
                if (key == 27 || key == 'q') // esc q
                {
                    return_flag = true;
                    break;
                }
                else if (key == 112) // p
                {
                    paused = !paused;
                }
            }
        }

        ++frameInd;
    }
    cv::destroyAllWindows();

    // save the result bounding box or not
    if(conf.mSaveBB && !conf.mUseCamera)
    {
        std::string bbFilePath = conf.mSeqBasePath+"/"+conf.mSeqName+"/"+conf.mBbFileName;
        writeResultBBFile(bbFilePath, rBBs);
    }
    // If there is ground truth and you want to evaluate the precision of the tracking result
    if(conf.mPrecision && gt_flag)
    {
        // evaluate presicion rate
        std::vector<float> prec = estPrecision(rBBs, gtBBs);
        std::string precFilePath = conf.mSeqBasePath+"/"+conf.mSeqName+"/"+conf.mPrecFileName;
        writePrecisionFile(precFilePath, prec);
    }

    tracker.Reset();
    cap.release();
    return true;
}

int main(int argc, char* argv[])
{
    std::string configPath;
    if(argc > 1)
    {
        configPath = argv[1];
    }
    else
    {
        configPath = "../../../config_mot.txt";
    }

    // read config file
    Config conf(configPath);

    // check config file
    if(!conf.check())
        return EXIT_FAILURE;

//    std::cout << conf << std::endl;

    if(conf.mUseCamera)
    {
        trackSequence(conf, 0);
    }
    else
    {
        int seqNum = conf.mSeqNames.size();
        for(int i=0; i<seqNum; ++i)
        {
            trackSequence(conf, i);
        }
    }

    return EXIT_SUCCESS;
}
