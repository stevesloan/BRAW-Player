#include "mainwindow.h"
#include "BlackmagicRawAPI.h"
#include <QApplication>
#include <QImage>
#include <QPixmap>
#include <QElapsedTimer>

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

MainWindow *w;

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

        BlackmagicRawResolutionScale res = 'qrtr';
        if (result == S_OK)
            result = frame->SetResolutionScale(res);

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

    virtual void ProcessComplete(IBlackmagicRawJob* job, HRESULT result, IBlackmagicRawProcessedImage* processedImage)
    {
        UserData* userData = nullptr;
        VERIFY(job->GetUserData((void**)&userData));

        std::cout << "Processed frame index: " << userData->frameIndex << std::endl;

        unsigned int width = 0;
        unsigned int height = 0;
        void* imageData = nullptr;
        if (result == S_OK)
            result = processedImage->GetWidth(&width);

        if (result == S_OK)
            result = processedImage->GetHeight(&height);

        if (result == S_OK)
            result = processedImage->GetResource(&imageData);

        unsigned char* rgba = static_cast<unsigned char*>(imageData);
        QImage qimage = QImage(rgba, static_cast<int>(width), static_cast<int>(height), QImage::Format_RGBA8888);
        QPixmap myPixmap = QPixmap::fromImage(qimage);
        w->setText(myPixmap, static_cast<int>(width), static_cast<int>(height));

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
    float frameRate = 0;
    long unsigned int frameIndex = 0;

    result = clip->GetFrameCount(&frameCount);
    result = clip->GetFrameRate(&frameRate);

    while (frameIndex < frameCount)
    {
        QElapsedTimer myTimer;
        myTimer.start();
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
        qApp->processEvents();
        w->updateProgress(((float)frameIndex / (float)frameCount) * 100);

        float delay = 1000 / frameRate;
        while(myTimer.elapsed() < delay) {
        }

    }

    return result;
}

void processFile(const char* clipName) {
    HRESULT result = S_OK;

    IBlackmagicRawFactory* factory = nullptr;
    IBlackmagicRaw* codec = nullptr;
    IBlackmagicRawClip* clip = nullptr;

    CameraCodecCallback callback;

    do
    {
        factory = CreateBlackmagicRawFactoryInstance();
        if (factory == nullptr)
        {
            std::cerr << "Failed to create IBlackmagicRawFactory!" << std::endl;
            result = E_FAIL;
            break;
        }

        result = factory->CreateCodec(&codec);
        if (result != S_OK)
        {
            std::cerr << "Failed to create IBlackmagicRaw!" << std::endl;
            break;
        }

        result = codec->OpenClip(clipName, &clip);
        if (result != S_OK)
        {
            std::cerr << "Failed to open IBlackmagicRawClip!" << std::endl;
            break;
        }

        result = codec->SetCallback(&callback);
        if (result != S_OK)
        {
            std::cerr << "Failed to set IBlackmagicRawCallback!" << std::endl;
            break;
        }

        result = ProcessClip(clip);

        codec->FlushJobs();

    } while(0);

    if (clip != nullptr)
        clip->Release();

    if (codec != nullptr)
        codec->Release();

    if (factory != nullptr)
        factory->Release();
}


int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " clipName.braw" << std::endl;
    }

    QApplication a(argc, argv);
    w = new MainWindow();
    w->show();

    std::cerr << argv[1] << std::endl;


    if(argc == 2) {
        const char* clipName = argv[1];
        processFile(clipName);
    }

    return a.exec();
}
