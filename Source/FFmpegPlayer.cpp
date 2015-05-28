#include "Stdafx.hpp"
#include "FFmpegPlayer.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <stdexcept>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include "Buffer.hpp"

namespace {

#define CHECK_FFMPEG_RETURN_CODE(ERROR_CODE, FUNCTION_NAME)  \
{   \
    if (ERROR_CODE < 0)	\
	{    \
        std::array<char, AV_ERROR_MAX_STRING_SIZE + 1> buffer;  \
\
        memset(buffer.data(), 0, buffer.size());    \
\
        int ret = av_strerror(m_videoStream, buffer.data(), buffer.size());   \
\
		std::string str = FUNCTION_NAME;	\
		str += ret == 0 ? buffer.data() : "unknown error";	\
		throw std::runtime_error(str);   \
    }   \
} while (false)

void free_av_frame(AVFrame* frame)
{
    avcodec_free_frame(&frame);
}

void close_av_input(AVFormatContext* context)
{
    avformat_close_input(&context);
}

}

CFFmpegPlayer::CFFmpegPlayer(const std::string& fileName): m_formatCtx(nullptr, close_av_input),
														   m_codecCtx(nullptr, avcodec_close),
														   m_frame(avcodec_alloc_frame(), free_av_frame),
														   m_swsCtx(nullptr), m_videoStream(-1)
{
    AVDictionary* optionsDict = nullptr;
    AVFormatContext* format = nullptr;

	int error = avformat_open_input(&format, fileName.c_str(), NULL, NULL);

    CHECK_FFMPEG_RETURN_CODE(error, "avformat_open_input");

    m_formatCtx.reset(format);

    error = avformat_find_stream_info(m_formatCtx.get(), NULL);

    CHECK_FFMPEG_RETURN_CODE(error, "avformat_find_stream_info");

    AVCodec* codec = nullptr;

    m_videoStream = av_find_best_stream(m_formatCtx.get(), AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);

    CHECK_FFMPEG_RETURN_CODE(error, "av_find_best_stream");

    assert(m_videoStream >= 0);

    m_codecCtx.reset(m_formatCtx->streams[m_videoStream]->codec);

    m_codecCtx->flags2 = 0;//CODEC_FLAG2_FAST;
    m_codecCtx->thread_count = 1;
    m_codecCtx->thread_type = 0;

    error = avcodec_open2(m_codecCtx.get(), codec, &optionsDict);

    CHECK_FFMPEG_RETURN_CODE(error, "avcodec_open2");

    m_outputWidth = 0;
    m_outputHeight = 0;
    m_pixelSize = 0;
    setOutputSize(m_codecCtx->width, m_codecCtx->height);
}

CFFmpegPlayer::~CFFmpegPlayer()
{
}

void CFFmpegPlayer::setOutputSize(int width, int height) {
    if (m_outputWidth == width && m_outputHeight == height)
        return;
    m_outputWidth = width;
    m_outputHeight = height;
    m_swsCtx = sws_getCachedContext(
        m_swsCtx, m_codecCtx->width, m_codecCtx->height,
        m_codecCtx->pix_fmt, width, height,
        AV_PIX_FMT_BGRA, SWS_POINT, nullptr, nullptr, nullptr);
    m_pixelSize = GetGLPixelSize(GL_BGRA);
}

int CFFmpegPlayer::getOutputSize() const
{
    return m_outputWidth * m_outputHeight * m_pixelSize;
}

bool CFFmpegPlayer::decodeFrame(unsigned int& pts, unsigned char* data, int lineSize)
{
    AVPacket packet;
    int frameFinished = 0;

    if (av_read_frame(m_formatCtx.get(), &packet) >= 0)
	{
        if (packet.stream_index == m_videoStream)
		{
            int error = avcodec_decode_video2(m_codecCtx.get(), m_frame.get(), &frameFinished, &packet);

			CHECK_FFMPEG_RETURN_CODE(error, "avcodec_decode_video2");

            if (frameFinished)
			{
                error = sws_scale(m_swsCtx, (unsigned char const * const *)m_frame->data, m_frame->linesize,
                                  0, m_codecCtx->height, &data, &lineSize);

				CHECK_FFMPEG_RETURN_CODE(error, "sws_scale");

                pts = av_q2d(m_formatCtx->streams[m_videoStream]->time_base) * m_frame->pts * 1000.0;

                av_free_packet(&packet);

				return true;
            }
        }
        av_free_packet(&packet);
    }
	else
	{
        // set the movie to the start again
        int error = av_seek_frame(m_formatCtx.get(), m_videoStream, 0,  AVSEEK_FLAG_FRAME);

		CHECK_FFMPEG_RETURN_CODE(error, "av_seek_frame");
    }

    return false;
}

void CFFmpegPlayer::initFFmpeg()
{
    av_register_all();
}
