#include "BmpAndWavAchieve.h"

bool saveBMP(const char *filePath, uint8_t *buffer, int width, int height, StorageFormat inputFormat)
{
    BMPFileHeader fileHeader;
    BMPInfoHeader infoHeader;

    infoHeader.width = width;
    infoHeader.height = -height;  // BMP 的图像数据默认从左下角开始，负值表示从左上角开始
    fileHeader.fileSize = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + width * height * 4;  // 每像素 4 字节

    std::ofstream outFile(filePath, std::ios::binary);
    if (!outFile)
        return false;

    // 写入文件头
    outFile.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));

    // 写入信息头
    outFile.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));

    // 写入像素数据（如果 buffer 是 RGBA 格式的，需要转换为 BMP 文件的 BGRA 顺序存储）
    if(inputFormat == StorageFormat::BGRA)
        outFile.write(reinterpret_cast<const char*>(buffer), width * height * 4);
    else if(inputFormat == StorageFormat::RGBA)
    {
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                uint8_t* pixel = buffer + (y * width + x) * 4;  // 4字节一个像素（RGBA）
                outFile.put(pixel[2]);  // B
                outFile.put(pixel[1]);  // G
                outFile.put(pixel[0]);  // R
                outFile.put(pixel[3]);  // A
            }
        }
    }

    outFile.close();
    return true;
}
