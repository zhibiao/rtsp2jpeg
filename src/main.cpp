#include <stdio.h>

extern "C" {
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libswscale/swscale.h"
    #include "libavutil/imgutils.h"
}

class AVContext {
public:
    AVContext(){}
    
    ~AVContext() {}
   
    int open(const char* url) {
        avformat_network_init();
        
        format_ctx = avformat_alloc_context();

        AVDictionary* options = NULL;
        av_dict_set(&options, "max_delay", "10000", 0);
        av_dict_set(&options, "buffer_size", "1024000", 0);
        av_dict_set(&options, "rtsp_transport", "tcp", 0);
        av_dict_set(&options, "analyzeduration", "1000000", 0);
        av_dict_set(&options, "stimeout","10000000", 0);
        
        if (avformat_open_input(&format_ctx,url,NULL,&options) != 0) {
           printf("%s failed!\n", "avformat_open_input");
           return -1;
        }
        
        if (avformat_find_stream_info(format_ctx,NULL) < 0) {
           printf("%s failed!\n", "avformat_find_stream_info");
           return -1;
        }
        
        video_index = av_find_best_stream(format_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
        if(video_index < 0){
           printf("%s failed!\n", "av_find_best_stream");
           return -1;
        }
        
        codec_ctx = avcodec_alloc_context3(NULL);
        avcodec_parameters_to_context(codec_ctx, format_ctx->streams[video_index]->codecpar);

        codec = avcodec_find_decoder(codec_ctx->codec_id);
        if(avcodec_open2(codec_ctx, codec, NULL)<0){
           printf("%s failed!\n", "avcodec_open2");
           return -1;
        }
        
        frame = av_frame_alloc();
        packet=(AVPacket *)av_malloc(sizeof(AVPacket));
        av_new_packet(packet, codec_ctx->width * codec_ctx->height);
        
        av_dump_format(format_ctx, 0, NULL, 0);
        
        out_codec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
        if (out_codec == NULL) {
           printf("%s failed!\n", "avcodec_find_encoder");
           return -1;
        }
    
        out_codec_ctx = avcodec_alloc_context3(out_codec);
        if (out_codec_ctx == NULL) {
           printf("%s failed!\n", "avcodec_alloc_context3");
           return -1;
        }
        
        out_codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
        out_codec_ctx->pix_fmt = AV_PIX_FMT_YUVJ420P;
        out_codec_ctx->width = codec_ctx->width; 
        out_codec_ctx->height = codec_ctx->height;
        out_codec_ctx->time_base.num = 1;
        out_codec_ctx->time_base.den = 60;  
        out_codec_ctx->qcompress = 0.5;  
        
        if (avcodec_open2(out_codec_ctx, out_codec, NULL) < 0){
           printf("%s failed!\n", "avcodec_open2");
           return -1;
        }
        
        out_packet=(AVPacket *)av_malloc(sizeof(AVPacket));
        av_new_packet(out_packet, out_codec_ctx->width * out_codec_ctx->height);
        
        return 0;
    }
    
    int read() {
        if(av_read_frame(format_ctx, packet) < 0) {
           av_packet_unref(packet);
           printf("%s failed!\n", "av_read_frame");
           return -2;
        }
        
        if(packet->stream_index != video_index) {
           av_packet_unref(packet);
           return -1;
        }
        
        if (avcodec_send_packet(codec_ctx, packet) < 0) {
           av_packet_unref(packet);
           return -1;
        }
        
        if (avcodec_receive_frame(codec_ctx, frame) < 0) {
           av_packet_unref(packet);
           return -1;
        }
        
        printf("yuv420p (%d,%d)\n", codec_ctx->width, codec_ctx->height);
        av_packet_unref(packet); 
      
        if (avcodec_send_frame(out_codec_ctx, frame) < 0) {
           av_packet_unref(out_packet);
           return -1;
        }
        
        if (avcodec_receive_packet(out_codec_ctx, out_packet) < 0) {
           av_packet_unref(out_packet);
           return -1;
        }
        
        printf("jpeg (%d,%d) size=%d\n", out_codec_ctx->width, out_codec_ctx->height, out_packet->size);
        av_packet_unref(out_packet);

        return 0;
    }
    
    void free() {
        if (frame) { av_frame_free(&frame); }

        if (format_ctx) { avformat_close_input(&format_ctx); }
       
        if (codec_ctx) { avcodec_free_context(&codec_ctx); }
    
        if (out_codec_ctx) { avcodec_free_context(&out_codec_ctx); }
    }
           
private:
    AVFormatContext *format_ctx = NULL;
    int video_index = -1;
    AVCodecContext *codec_ctx = NULL;
    AVCodec *codec = NULL;
    AVFrame  *frame = NULL;
    AVPacket *packet = NULL;
    AVCodecContext *out_codec_ctx = NULL;
    AVCodec *out_codec = NULL;
    AVPacket *out_packet = NULL;
};

int main(int argc, char *argv) {
    AVContext ctx = AVContext();
    ctx.open("rtsp://10.144.176.206/ch0/main");
    for (;;) {
        if (ctx.read() == -2) {
            break;
        }
    }
    ctx.free();
}
