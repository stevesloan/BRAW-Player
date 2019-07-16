#include "mainwindow.h"
#include "BlackmagicRawAPI.h"
#include <QApplication>

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#ifdef DEBUG
    #include <cassert>
    #define VERIFY(condition) assert(SUCCEEDED(condition))
#else
    #define VERIFY(condition) condition
#endif

static const BlackmagicRawResourceFormat s_resourceFormat = blackmagicRawResourceFormatRGBAU8;
static const int s_maxJobsInFlight = 3;
static std::atomic<int> s_jobsInFlight = {0};

struct UserData
{
    unsigned long long frameIndex;
};

class CameraCodecCallback : public IBlackmagicRawCallback
{
public:
    explicit CameraCodecCallback() = default;
    virtual ~CameraCodecCallback() = default;

    virtual void ReadComplete(IBlackmagicRawJob* readJob, HRESULT result, IBlackmagicRawFrame* frame)
    {
        UserData* userData = nullptr;
        VERIFY(readJob->GetUserData((void**)&userData));

        IBlackmagicRawJob* decodeAndProcessJob = nullptr;

        if (result == S_OK)
            VERIFY(frame->SetResourceFormat(s_resourceFormat));

        if (result == S_OK)
            result = frame->CreateJobDecodeAndProcessFrame(nullptr, nullptr, &decodeAndProcessJob);

        if (result == S_OK)
            VERIFY(decodeAndProcessJob->SetUserData(userData));

        if (result == S_OK)
            result = decodeAndProcessJob->Submit();

        if (result != S_OK)
        {
            if (decodeAndProcessJob)
                decodeAndProcessJob->Release();

            delete userData;
        }

        readJob->Release();
    }

    virtual void ProcessComplete(IBlackmagicRawJob* job, HRESULT result, IBlackmagicRawProcessedImage*)
    {
        UserData* userData = nullptr;
        VERIFY(job->GetUserData((void**)&userData));

        std::cout << "Processed frame index: " << userData->frameIndex << std::endl;
        delete userData;

        job->Release();
        --s_jobsInFlight;
    }

    virtual void DecodeComplete(IBlackmagicRawJob*, HRESULT) {}
    virtual void TrimProgress(IBlackmagicRawJob*, float) {}
    virtual void TrimComplete(IBlackmagicRawJob*, HRESULT) {}
    virtual void SidecarMetadataParseWarning(IBlackmagicRawClip*, const char*, uint32_t, const char*) {}
    virtual void SidecarMetadataParseError(IBlackmagicRawClip*, const char*, uint32_t, const char*) {}

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID*)
    {
        return E_NOTIMPL;
    }

    virtual ULONG STDMETHODCALLTYPE AddRef(void)
    {
        return 0;
    }

    virtual ULONG STDMETHODCALLTYPE Release(void)
    {
        return 0;
    }
};

HRESULT ProcessClip(IBlackmagicRawClip* clip)
{
    HRESULT result;

    long unsigned int frameCount = 0;
    long unsigned int frameIndex = 0;

    result = clip->GetFrameCount(&frameCount);

    while (frameIndex < frameCount)
    {
        if (s_jobsInFlight >= s_maxJobsInFlight)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            continue;
        }

        IBlackmagicRawJob* jobRead = nullptr;
        if (result == S_OK)
            result = clip->CreateJobReadFrame(frameIndex, &jobRead);

        UserData* userData = nullptr;
        if (result == S_OK)
        {
            userData = new UserData();
            userData->frameIndex = frameIndex;
            VERIFY(jobRead->SetUserData(userData));
        }

        if (result == S_OK)
            result = jobRead->Submit();

        if (result != S_OK)
        {
            if (userData != nullptr)
                delete userData;

            if (jobRead != nullptr)
                jobRead->Release();

            break;
        }

        ++s_jobsInFlight;

        frameIndex++;
    }

    return result;
}

int main(int argc, char *argv[])
{
//    if (argc > 2)
//        {
//            std::cerr << "Usage: " << argv[0] << " clipName.braw" << std::endl;
//            return 1;
//        }

//        const char* clipName = nullptr;
//        bool clipNameProvided = argc == 2;
//        if (clipNameProvided)
//        {
//            clipName = argv[1];
//        }
//        else
//        {
//            clipName = "../../../Media/sample.braw";
//        }

//        HRESULT result = S_OK;

//        IBlackmagicRawFactory* factory = nullptr;
//        IBlackmagicRaw* codec = nullptr;
//        IBlackmagicRawClip* clip = nullptr;

//        CameraCodecCallback callback;

//        do
//        {
//            factory = CreateBlackmagicRawFactoryInstanceFromPath("./Libraries/");
//            if (factory == nullptr)
//            {
//                std::cerr << "Failed to create IBlackmagicRawFactory!" << std::endl;
//                result = E_FAIL;
//                break;
//            }

//            result = factory->CreateCodec(&codec);
//            if (result != S_OK)
//            {
//                std::cerr << "Failed to create IBlackmagicRaw!" << std::endl;
//                break;
//            }

//            result = codec->OpenClip(clipName, &clip);
//            if (result != S_OK)
//            {
//                std::cerr << "Failed to open IBlackmagicRawClip!" << std::endl;
//                break;
//            }

//            result = codec->SetCallback(&callback);
//            if (result != S_OK)
//            {
//                std::cerr << "Failed to set IBlackmagicRawCallback!" << std::endl;
//                break;
//            }

//            result = ProcessClip(clip);
//            codec->FlushJobs();

//        } while(0);

//        if (clip != nullptr)
//            clip->Release();

//        if (codec != nullptr)
//            codec->Release();

//        if (factory != nullptr)
//            factory->Release();


    //////////
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
