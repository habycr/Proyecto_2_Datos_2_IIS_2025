//
// Created by dylan on 22/11/25.
//

#pragma once
#include <string>
#include <vector>

using namespace std;

struct AnalysisResult {
    bool success;
    string complexity;
    string algorithmType;
    int nestedLoops;
    bool isRecursive;
    double averageRatio;
    vector<double> executionTimes;
    string explanation;
    vector<string> suggestions;
    string errorMessage;
    string consoleOutput;
};

class ComplexityAnalyzer {
private:
    string code;
    string functionName;
    string geminiApiKey;

public:
    ComplexityAnalyzer(const string& sourceCode, const string& apiKey = "");
    AnalysisResult analyze();

private:


    string extractFunctionName();
    int countNestedLoops();
    bool detectRecursion();
    bool detectLogarithmic();
    string identifyAlgorithmType();

    string determineComplexity(const vector<double>& times);
    string generateExplanation(const string& complexity, const string& type);
    vector<string> generateSuggestions(const string& complexity);

    vector<string> generateSuggestionsWithGemini(const string& complexity, const string& consoleOutput);

};
