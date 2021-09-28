#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;
using namespace std::filesystem;

path _pricePath = "price";
int _closeCol = 4;
string _testStartYear = "2012";
string _testEndYear = "2021";
int _testYearLength = stod(_testEndYear) - stod(_testStartYear);
string _slidingWindows[] = {"YY2Y", "YH2Y", "Y2Y", "Y2H", "Y2Q", "Y2M", "H#", "H2H", "H2Q", "H2M", "Q#", "Q2Q", "Q2M", "M#", "M2M", "A2A", "10D5"};
string _slidingWindowsEX[] = {"24M12", "18M12", "12M12", "12M6", "12M3", "12M1", "6M", "6M6", "6M3", "6M1", "3M", "3M3", "3M1", "1M", "1M1", "A2A", "10D5"};
int _MAType = 0;

vector<vector<string> > read_data(path);
vector<path> get_path(path);

class CompanyInfo {
public:
    class MATable {
    public:
        int trainDays;
        string *date;
        double *price;
        double **MAValues;
        ~MATable();
    };
    string companyName;
    string MAType;
    string *date;
    double *price;
    int totalDays;
//    int trainDays;
    string MAOutputPath;
    int testStartRow;
    int testEndRow;
    int trainStartRow;
    int trainEndRow;
    vector<vector<int> > trainInterval;
    
    void store_date_price(path);
    string create_folder();
    void cal_MA();
    void train();
    void find_train_interval();
    void find_train_start_row(int, char);
    void find_train_start_end(vector<string>, char);
    vector<string> find_train_type(string, char &);
    MATable create_MATable();
//    void output_MA(path, int);
    void find_cross(int, int);
    CompanyInfo(path filePath, string MAType) {
        companyName = filePath.stem().string();
        store_date_price(filePath);
        this->MAType = MAType;
        MAOutputPath = create_folder();
    }
    ~CompanyInfo();
};

string CompanyInfo::create_folder() {
    create_directories(MAType + "/" + companyName);
    return MAType + "/" + companyName;
}

void CompanyInfo::store_date_price(path priceFilePath) {
    vector<vector<string> > priceFile = read_data(priceFilePath);
    totalDays = (int)priceFile.size() - 1;
    date = new string[totalDays];
    price = new double[totalDays];
    for (int i = 1, j = 0; i <= totalDays; i++) {
        date[i - 1] = priceFile[i][0];
        if (priceFile[i][_closeCol] == "null") {
            price[i - 1] = 0;
        }
        else {
            price[i - 1] = stod(priceFile[i][_closeCol]);
        }
        if (j == 0 && date[i - 1].substr(0, 4) == _testStartYear) {
            testStartRow = i - 1;
            j++;
        }
        else if (j == 1 && date[i - 1].substr(0, 4) == _testEndYear) {
            testEndRow = i - 2;
            j++;
        }
    }
}

void CompanyInfo::cal_MA() {
    cout << "calculating " << companyName + " " + MAType << endl;
    switch (MAType[0]) {
        case 'S':
            for (int MA = 1; MA < 257; MA++) {
                ofstream out;
                if (MA < 10) {
                    out.open(MAOutputPath + "/" + companyName + "_" + MAType + "_00" + to_string(MA) + ".csv");
                }
                else if (MA >= 10 && MA < 100) {
                    out.open(MAOutputPath + "/" + companyName + "_" + MAType + "_0" + to_string(MA) + ".csv");
                }
                else if (MA >= 100) {
                    out.open(MAOutputPath + "/" + companyName + "_" + MAType + "_" + to_string(MA) + ".csv");
                }
                for (int dateRow = MA - 1; dateRow < totalDays; dateRow++) {
                    double MARangePriceSum = 0;
                    for (int i = dateRow, j = MA; j > 0; i--, j--) {
                        MARangePriceSum += price[i];
                    }
                    out << fixed << setprecision(8) << date[dateRow] + "," << MARangePriceSum / MA << endl;
                }
                out.close();
            }
            break;
        case 'W':
            break;
        case 'E':
            break;
    }
}

void CompanyInfo::train() {
}

void CompanyInfo::find_train_interval() {
    for (int windowsIndex = 0; windowsIndex < sizeof(_slidingWindows) / sizeof(_slidingWindows[0]); windowsIndex++) {
        char delimiter;
        vector<string> trainType = find_train_type(_slidingWindowsEX[windowsIndex], delimiter);
        if (_slidingWindows[windowsIndex] == "A2A") {
            vector<int> tmp;
            tmp.push_back(testStartRow);
            tmp.push_back(testEndRow);
            trainInterval.push_back(tmp);
        }
        else {
            find_train_start_end(trainType, delimiter);
        }
        //        cout << _slidingWindows[windowsIndex] << endl;
        //        for (int i = 0; i < trainInterval[windowsIndex].size(); i += 2) {
        //            cout << date[trainInterval[windowsIndex][i]] << "~" << date[trainInterval[windowsIndex][i + 1]] << endl;
        //        }
    }
}

void CompanyInfo::find_train_start_row(int trainPeriodLength, char delimiter) {
    trainStartRow = -1;
    trainEndRow = -1;
    if (delimiter == 'M') {
        for (int i = testStartRow - 1, monthCount = 0; i >= 0; i--) {
            if (date[i].substr(5, 2) != date[i - 1].substr(5, 2)) {
                monthCount++;
                if (monthCount == trainPeriodLength) {
                    trainStartRow = i;
                    break;
                }
            }
        }
    }
    else if (delimiter == 'D') {
        trainStartRow = testStartRow - trainPeriodLength;
        trainEndRow = testStartRow - 1;
    }
    if (trainStartRow == -1) {
        cout << companyName + " can not find trainStartRow " << trainPeriodLength << endl;
        exit(1);
    }
}

void CompanyInfo::find_train_start_end(vector<string> trainType, char delimiter) {
    vector<int> startRow;
    vector<int> endRow;
    vector<int> allRow;
    int trainPeriodLength = stoi(trainType[0]);
    int intervalNum = -1;
    int testPeriodLength = -1;
    //=======================================找出訓練期開始Row
    if (trainType.size() == 2) {
        find_train_start_row(trainPeriodLength, delimiter);
        intervalNum = _testYearLength * (12 / stoi(trainType[1]));
        testPeriodLength = stoi(trainType[1]);
    }
    else if (trainType.size() == 1) {
        find_train_start_row(12, delimiter);
        intervalNum = _testYearLength * (12 / stoi(trainType[0]));
        testPeriodLength = stoi(trainType[0]);
    }
    //=======================================找出所有訓練區間
    if (delimiter == 'M') {
        startRow.push_back(trainStartRow);
        for (int i = trainStartRow, intervalCount = 1, monthCount = 0; intervalCount < intervalNum; i++) {
            if (date[i].substr(5, 2) != date[i + 1].substr(5, 2)) {
                monthCount++;
                if (monthCount == testPeriodLength) {
                    startRow.push_back(i + 1);
                    intervalCount++;
                    monthCount = 0;
                }
            }
        }
        if (trainType.size() == 2) {
            endRow.push_back(testStartRow - 1);
            trainEndRow = testStartRow;
        }
        else if (trainType.size() == 1) {
            for (int i = trainStartRow, monthCount = 0; i < totalDays; i++) {
                if (date[i].substr(5, 2) != date[i + 1].substr(5, 2)) {
                    monthCount++;
                    if (monthCount == trainPeriodLength) {
                        endRow.push_back(i);
                        trainEndRow = i + 1;
                        break;
                    }
                }
            }
        }
        for (int i = trainEndRow, intervalCount = 1, monthCount = 0; intervalCount < intervalNum; i++) {
            if (date[i].substr(5, 2) != date[i + 1].substr(5, 2)) {
                monthCount++;
                if (monthCount == testPeriodLength) {
                    endRow.push_back(i);
                    intervalCount++;
                    monthCount = 0;
                }
            }
        }
    }
    else if (delimiter == 'D') {
        for (int i = trainStartRow; i <= testEndRow - trainPeriodLength; i += testPeriodLength) {
            startRow.push_back(i);
        }
        for (int i = trainEndRow; i < testEndRow; i += testPeriodLength) {
            endRow.push_back(i);
        }
    }
    for (int i = 0; i < startRow.size(); i++) {
        allRow.push_back(startRow[i]);
        allRow.push_back(endRow[i]);
    }
    //=======================================
    trainInterval.push_back(allRow);
}

vector<string> CompanyInfo::find_train_type(string window, char &delimiter) {
    for (int i = 0; i < window.length(); i++) {
        if (isalpha(window[i])) {
            delimiter = window[i];
            break;
        }
    }
    string segment;
    vector<string> segmentList;
    stringstream toCut(window);
    while (getline(toCut, segment, delimiter)) {
        segmentList.push_back(segment);
    }
    return segmentList;
}

CompanyInfo::MATable CompanyInfo::create_MATable() {
    int longestTrainMonth = 0;
    for (int i = 0; i < sizeof(_slidingWindowsEX) / sizeof(_slidingWindowsEX[0]); i++) {
        char delimiter;
        vector<string> tmp = find_train_type(_slidingWindowsEX[i], delimiter);
        if (delimiter == 'M' && stoi(tmp[0]) > longestTrainMonth) {
            longestTrainMonth = stoi(tmp[0]);
        }
    }
    find_train_start_row(longestTrainMonth, 'M');
    MATable table;
    table.trainDays = totalDays - trainStartRow;
    table.date = new string[table.trainDays];
    table.price = new double[table.trainDays];
    for (int i = trainStartRow, j = 0; i < totalDays; i++, j++) {
        table.date[j] = date[i];
        table.price[j] = price[i];
    }
    table.MAValues = new double*[table.trainDays];
    for(int i = 0; i < table.trainDays; i++) {
        table.MAValues[i] = new double[257];
    }
    vector<path> MAFilePath = get_path(MAType + "/" + companyName);
    for(int i = 0; i < MAFilePath.size(); i++) {
        vector<vector<string> > MAFile = read_data(MAFilePath[i]);
        for (int j = 0, k = int(MAFile.size()) - table.trainDays; k < MAFile.size(); j++, k++) {
            table.MAValues[j][i + 1] = stod(MAFile[k][1]);
        }
    }
//    ofstream out;
//    out.open("testTable.csv");
//    for(int i = 0; i < table.trainDays; i++) {
//        out << table.date[i] + ",";
//        for(int j = 1; j < 257; j++) {
//            out << table.MAValues[i][j] << ",";
//        }
//        out << endl;
//    }
    return table;
}

//void CompanyInfo::output_MA(path filePath, int MAType) {
//    string type;
//    switch (MAType) {
//        case 0:
//            type = "_SMA.csv";
//            break;
//        case 1:
//            type = "_WMA.csv";
//            break;
//        case 2:
//            type = "_EMA.csv";
//            break;
//    }
//
//    ofstream MAOut;
//    MAOut.open(filePath.stem().string() + type);
//    MAOut << ",";
//    for (int i = 1; i < 257; i++) {
//        MAOut << i << ",";
//    }
//    MAOut << endl;
//    for (int i = 0, dateRow = trainStartRow; i < trainDays; i++, dateRow++) {
//        MAOut << date[dateRow] + ",";
//        for (int j = 1; j < 257; j++) {
//            //            MAOut << fixed << setprecision(8) << MATable[i][j] << ",";
//        }
//        MAOut << endl;
//    }
//    MAOut.close();
//}

void CompanyInfo::find_cross(int high, int low) {}

CompanyInfo::~CompanyInfo() {
    delete[] date;
    delete[] price;
}

CompanyInfo::MATable::~MATable() {
    delete [] date;
    for(int i = 0; i < trainDays; i++) {
        delete [] MAValues[i];
    }
    delete [] MAValues;
}

vector<vector<string> > read_data(path filePath) {
    ifstream infile(filePath);
    vector<vector<string> > data;
    if (!infile) {
        cout << filePath.string() + " not found" << endl;
        exit(1);
    }
    cout << "reading " + filePath.filename().string() << endl;
    string row;
    string cell;
    vector<string> oneRow;
    while (infile) {
        getline(infile, row);
        stringstream lineStream(row);
        if (row.length() != 0) {
            while (getline(lineStream, cell, ',')) {
                oneRow.push_back(cell);
            }
            data.push_back(oneRow);
        }
        row.clear();
        cell.clear();
        vector<string>().swap(oneRow);
    }
    infile.close();
    return data;
}

vector<path> get_path(path targetPath) {
    vector<path> filePath;
    copy(directory_iterator(targetPath), directory_iterator(), back_inserter(filePath));
    sort(filePath.begin(), filePath.end());
    return filePath;
}

int main(int argc, const char *argv[]) {
    string MAType[] = {"SMA", "WMA", "EMA"};
    vector<path> companyPricePath = get_path(_pricePath);
    for (int companyIndex = 0; companyIndex < 1; companyIndex++) {
        CompanyInfo company(companyPricePath[companyIndex], MAType[_MAType]);
//        company.cal_MA();
//        company.find_train_interval();
        CompanyInfo::MATable table = company.create_MATable();
    }
    return 0;
}
