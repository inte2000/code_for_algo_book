// knn-digital.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <filesystem>
#include <functional>


namespace std_fs = std::experimental::filesystem;

const int BMP_WIDTH = 32;
const int BMP_HEIGHT = 32;
const int NUM_COUNT = 10; // 0-9
const int K = 9;

const std::string FILE_PATH = "..\\";

typedef struct
{
    int cat;
    char vec[BMP_WIDTH * BMP_HEIGHT];
}SampleVec;

typedef struct
{
    double distance;
    int cat;
}CatResult;

std::pair<bool, int> GetCategoryFromFileName(const std::string fileName)
{
    std::size_t pos = fileName.find('_');
    if (pos == std::string::npos)
    {
        return {false, 0};
    }

    int cat = fileName[pos - 1] - '0';

    return { true, cat };
}

bool AppendToVec(SampleVec& vec, int row, std::string& sline)
{
    if (sline.length() != BMP_WIDTH)
    {
        return false;
    }

    char *pvs = vec.vec + row * BMP_WIDTH;
    for (std::size_t i = 0; i < sline.length(); i++)
    {
        *pvs++ = sline[i] - '0';
    }

    return true;
}

bool LoadFileToVec(const std::string fileName, SampleVec& vec)
{
    std::ifstream file(fileName, std::ios::in);
    if (!file)
    {
        return false;
    }
    int row = 0;
    std::string sline;
    while (std::getline(file, sline))
    {
        if (!AppendToVec(vec, row, sline))
        {
            break;
        }
        row++;
    }
    
    return (row == BMP_HEIGHT);
}

double ManhattanDustance(const SampleVec& vec1, const SampleVec& vec2)
{
    double total = 0.0;
    for (int i = 0; i < BMP_WIDTH * BMP_HEIGHT; i++)
    {
        total += std::abs(vec1.vec[i] - vec2.vec[i]);
    }

    return total;
}

bool LoadDataSet(const std::string filePath, std::vector<SampleVec>& dataSet)
{
    for (auto& p : std_fs::directory_iterator(filePath))
    {
        std::string fileName = p.path().generic_string();
        std::pair<bool, int> catrtn = GetCategoryFromFileName(fileName);
        if (!catrtn.first)
        {
            return false;
        }

        SampleVec vec = { catrtn.second };
        if (!LoadFileToVec(fileName, vec))
        {
            return false;
        }

        dataSet.emplace_back(vec); //better than push_back()
    }

    return true;
}

int GetMaxCountCategory(const std::vector<int>& count)
{
    int mj = 0;
    for (int j = 1; j < NUM_COUNT; j++)
    {
        if (count[j] > count[mj])
        {
            mj = j;
        }
    }

    return mj;
}

int Classify(const std::vector<SampleVec>& dataTrain, const SampleVec& vec)
{
    int idx = 0;
    std::vector<CatResult> cr(dataTrain.size());
    for (auto& vt : dataTrain)
    {
        cr[idx].cat = vt.cat;
        cr[idx++].distance = ManhattanDustance(vt, vec);
    }

    auto lessCrPred = [](const CatResult& cr1, const CatResult& cr2)->bool { return (cr1.distance < cr2.distance); };
    std::sort(cr.begin(), cr.end(), lessCrPred); 
    
    std::vector<int> count(NUM_COUNT, 0); 
    for (int i = 0; i < K; i++) 
    {
        count[cr[i].cat]++;
    }

    return GetMaxCountCategory(count);
}

int main()
{
    const std::string trainFile = FILE_PATH + "traindata";
    std::vector<SampleVec> dataTrain;
    if (!LoadDataSet(trainFile, dataTrain))
    {
        std::cout << "Fail to load trainning data file!" << std::endl;
        return 0;
    }

    const std::string testFile = FILE_PATH + "testdata";
    std::vector<SampleVec> dataTest;
    if(!LoadDataSet(testFile, dataTest))
    {
        std::cout << "Fail to load test data file!" << std::endl;
        return 0;
    }

    std::vector<std::pair<int, int>> result(NUM_COUNT);
    for(auto& v : dataTest)
    {
        result[v.cat].first++;
        int cat = Classify(dataTrain, v);
        if (cat == v.cat)
        {
            result[v.cat].second++;
        }
    }

    for (std::size_t i = 0; i < result.size(); i++)
    {
        double rato = double(result[i].second) * 100.0 / result[i].first;
        std::cout << "category : " << i << "  success : " << std::setprecision(4) << rato << std::endl;
    }
    return 0;
}
