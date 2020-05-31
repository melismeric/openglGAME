#pragma once
#include "GLTexture.h"
#include <string>
#include <vector>

using namespace std;


class ImageLoader
{
public:
	static GLTexture loadPNG(std::string filePath);
	static GLTexture loadCubemap(vector<string> faces);
};

