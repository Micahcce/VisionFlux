#ifndef BMPANDWAVACHIEVE_H
#define BMPANDWAVACHIEVE_H

#include <fstream>  // 用于写入 BMP 文件

// 像素存储格式
enum StorageFormat{
    RGBA = 0,
    BGRA
};

// 定义 BMP 文件头和信息头结构
#pragma pack(push, 1)
struct BMPFileHeader {
    uint16_t fileType{0x4D42};  // 文件类型 'BM'
    uint32_t fileSize{0};       // 文件大小
    uint16_t reserved1{0};      // 保留字段
    uint16_t reserved2{0};      // 保留字段
    uint32_t offsetData{54};    // 图像数据的偏移量
};

struct BMPInfoHeader {
    uint32_t size{40};           // 结构体大小
    int32_t width{0};            // 图像宽度
    int32_t height{0};           // 图像高度
    uint16_t planes{1};          // 颜色平面数（总是为1）
    uint16_t bitCount{32};       // 每个像素的位数（这里是32位RGB格式）
    uint32_t compression{0};     // 压缩方式（0表示不压缩）
    uint32_t sizeImage{0};       // 图像大小（不压缩时可以为0）
    int32_t xPixelsPerMeter{0};  // 水平分辨率
    int32_t yPixelsPerMeter{0};  // 垂直分辨率
    uint32_t colorsUsed{0};      // 使用的颜色数
    uint32_t colorsImportant{0}; // 重要的颜色数
};
#pragma pack(pop)

bool saveBMP(const char* filePath, uint8_t* buffer, int width, int height, StorageFormat inputFormat);

#endif // BMPANDWAVACHIEVE_H
