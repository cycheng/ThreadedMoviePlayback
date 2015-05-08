#ifndef FFMPEGPLAYER_HPP
#define FFMPEGPLAYER_HPP

#include <memory>
#include <string>

/**
 * @brief The CFFmpegPlayer class
 * @reentrant
 */
class CFFmpegPlayer
{
public:
    /**
     * Opens the movie file. Throws exception on failure.
     */
    explicit CFFmpegPlayer(const std::string& fileName);
    ~CFFmpegPlayer();

    /**
     * Ininitalisize ffmpeg, must be called before CFFmpegPlayer instances are created.
     */
    static void initFFmpeg();

    /**
     * Tries to decode a frame. Throws execption on failure. Frame is decoded as BGRA 8 bit per channel.
     * @param pts Present time of the decoded frame.
     * @param data Pointer to the memory where to store the decoded data.
     * @param lineSize Line size to use for the decoded data.
     * @returns True if a new frame was decoded, else false.
     */
    bool decodeFrame(unsigned int& pts, unsigned char* data, int lineSize);

    void setOutputSize(int width, int height);
    int getOutputSize() const;

private:
    std::unique_ptr<struct AVFormatContext, void (*)(struct AVFormatContext*)> m_formatCtx;
    std::unique_ptr<struct AVCodecContext, int (*)(struct AVCodecContext*)> m_codecCtx;
    std::unique_ptr<struct AVFrame, void (*)(struct AVFrame*)> m_frame;
    struct SwsContext* m_swsCtx;
    int m_videoStream;

    int m_outputWidth;
    int m_outputHeight;
};

#endif // FFMPEGPLAYER_HPP
