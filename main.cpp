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
string _slidingWindowsEX[] = {"24M12", "18M12", "12M12", "12M6", "12M3", "12M1", "H#", "6M6", "6M3", "6M1", "Q#", "3M3", "3M1", "M#", "1M1", "A2A", "10D5"};
int _MAType = 0;

vector<vector<string> > read_data(path);
vector<path> get_path(path);

class CompanyInfo {
   public:
    string companyName;
    string *date;
    double *price;
    int totalDays;
    string MAOutputPath;
    int testStartRow;
    int testEndRow;
    int trainStartRow;
    int trainEndRow;
    int trainDays;
    double **MATable;
    vector<vector<int> > trainInterval;

    void store_date_price(path);
    string create_folder(string);
    void cal_MA(string);
    void train();
    void find_train_interval();
    void find_train_start_row(int);
    void find_train_start_end(int, int, int, int);
    vector<string> find_train_type(string, char, char);
    void ini_MATable();
    void output_MA(path, int);
    void find_cross(int, int);
    CompanyInfo(path filePath, string MAType) {
        companyName = filePath.stem().string();
        store_date_price(filePath);
        MAOutputPath = create_folder(MAType);
    }
    ~CompanyInfo();
};

string CompanyInfo::create_folder(string MAType) {
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

void CompanyInfo::cal_MA(string MAType) {
    switch (MAType[0]) {
        case 'S':
            for (int MA = 1; MA < 257; MA++) {
                cout << MA << endl;
                ofstream out;
                out.open(MAOutputPath + "/" + companyName + "_" + MAType + "_" +
                         to_string(MA) + ".csv");
                for (int dateRow = MA - 1; dateRow < totalDays; dateRow++) {
                    double MARangePriceSum = 0;
                    for (int i = dateRow, j = MA; j > 0; i--, j--) {
                        MARangePriceSum += price[i];
                    }
                    out << fixed << setprecision(8) << date[dateRow] + ","
                        << MARangePriceSum / MA << endl;
                }
                out.close();
            }
            break;
        case 'W':
            for (int MA = 1; MA < 257; MA++) {
                for (int dateRow = trainStartRow, MARow = 0; dateRow <= trainEndRow;
                     dateRow++, MARow++) {
                    double MARangePriceSum = 0;
                    int MASum = 0;
                    int MAWeight = MA;
                    for (int MADays = 0, priceRow = dateRow; MADays < MA;
                         MADays++, priceRow--, MASum += MAWeight, MAWeight--) {
                        MARangePriceSum += price[priceRow] * MAWeight;
                    }
                    MATable[MARow][MA] = MARangePriceSum / MASum;
                }
            }
            break;
        case 'E':
            break;
    }
}

void CompanyInfo::train() {
    find_train_interval();
}

void CompanyInfo::find_train_interval() {
    trainInterval.resize(sizeof(_slidingWindows) / sizeof(_slidingWindows[0]));
    for (int windowsIndex = 0; windowsIndex < sizeof(_slidingWindows) / sizeof(_slidingWindows[0]); windowsIndex++) {
        if (_slidingWindows[windowsIndex] == "YY2Y") {
            find_train_start_end(24, _testYearLength, 12, windowsIndex);
        }
        else if (_slidingWindows[windowsIndex] == "YH2Y") {
            find_train_start_end(18, _testYearLength, 12, windowsIndex);
        }
        else if (_slidingWindows[windowsIndex] == "Y2Y") {
            find_train_start_end(12, _testYearLength, 12, windowsIndex);
        }
        else if (_slidingWindows[windowsIndex] == "Y2H") {
            find_train_start_end(12, _testYearLength * 2, 6, windowsIndex);
        }
        else if (_slidingWindows[windowsIndex] == "Y2Q") {
            find_train_start_end(12, _testYearLength * 4, 3, windowsIndex);
        }
        else if (_slidingWindows[windowsIndex] == "Y2M") {
            find_train_start_end(12, _testYearLength * 12, 1, windowsIndex);
        }
        else if (_slidingWindows[windowsIndex] == "H#") {
            find_train_start_end(6, _testYearLength * 2, 6, windowsIndex);
        }
        else if (_slidingWindows[windowsIndex] == "H2H") {
            find_train_start_end(6, _testYearLength * 2, 6, windowsIndex);
        }
        else if (_slidingWindows[windowsIndex] == "H2Q") {
            find_train_start_end(6, _testYearLength * 4, 3, windowsIndex);
        }
        else if (_slidingWindows[windowsIndex] == "H2M") {
            find_train_start_end(6, _testYearLength * 12, 1, windowsIndex);
        }
        else if (_slidingWindows[windowsIndex] == "Q#") {
            find_train_start_end(3, _testYearLength * 4, 3, windowsIndex);
        }
        else if (_slidingWindows[windowsIndex] == "Q2Q") {
            find_train_start_end(3, _testYearLength * 4, 3, windowsIndex);
        }
        else if (_slidingWindows[windowsIndex] == "Q2M") {
            find_train_start_end(3, _testYearLength * 12, 1, windowsIndex);
        }
        else if (_slidingWindows[windowsIndex] == "M#") {
            find_train_start_end(1, _testYearLength * 12, 1, windowsIndex);
        }
        else if (_slidingWindows[windowsIndex] == "M2M") {
            find_train_start_end(1, _testYearLength * 12, 1, windowsIndex);
        }
        else if (_slidingWindows[windowsIndex] == "A2A") {
            trainInterval[windowsIndex].push_back(testStartRow);
            trainInterval[windowsIndex].push_back(testEndRow);
        }
        else if (isnumber(_slidingWindows[windowsIndex][0])) {
            find_train_start_end(-1, -1, -1, windowsIndex);
        }
        else {
            cout << _slidingWindows[windowsIndex] + " is not defined" << endl;
            exit(0);
        }
        cout << _slidingWindows[windowsIndex] << endl;
        for (int i = 0; i < trainInterval[windowsIndex].size(); i += 2) {
            cout << date[trainInterval[windowsIndex][i]] << "~" << date[trainInterval[windowsIndex][i + 1]] << endl;
        }
    }
}

void CompanyInfo::find_train_start_row(int trainPeriodLength) {
    trainStartRow = -1;
    trainEndRow = -1;
    for (int i = testStartRow - 1, monthCount = 0; i >= 0; i--) {
        if (date[i].substr(5, 2) != date[i - 1].substr(5, 2)) {
            monthCount++;
            if (monthCount == trainPeriodLength) {
                trainStartRow = i;
                break;
            }
        }
    }
    if (trainStartRow == -1) {
        cout << companyName + " can not find trainStartRow " << trainPeriodLength << endl;
        exit(1);
    }
}

void CompanyInfo::find_train_start_end(int trainPeriodLength, int intervalNum, int testPeriodLength, int windowsIndex) {
    vector<int> startRow;
    vector<int> endRow;
    if (isalpha(_slidingWindows[windowsIndex][0])) {
        if (_slidingWindows[windowsIndex].length() != 2) {
            find_train_start_row(trainPeriodLength);
        }
        else {
            find_train_start_row(12);
        }
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
        if (_slidingWindows[windowsIndex].length() != 2) {
            endRow.push_back(testStartRow - 1);
            trainEndRow = testStartRow;
        }
        else {
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
        for (int i = 0; i < intervalNum; i++) {
            trainInterval[windowsIndex].push_back(startRow[i]);
            trainInterval[windowsIndex].push_back(endRow[i]);
        }
    }
    else {
        trainPeriodLength = stoi(find_train_type(_slidingWindows[windowsIndex], 'D', '!')[0]);
        testPeriodLength = stoi(find_train_type(_slidingWindows[windowsIndex], 'D', '!')[1]);
        trainStartRow = testStartRow - trainPeriodLength;
        trainEndRow = testStartRow - 1;
        for (int i = trainStartRow; i <= testEndRow - trainPeriodLength; i += testPeriodLength) {
            startRow.push_back(i);
        }
        for (int i = trainEndRow; i < testEndRow; i += testPeriodLength) {
            endRow.push_back(i);
        }
        for (int i = 0; i < startRow.size(); i++) {
            trainInterval[windowsIndex].push_back(startRow[i]);
            trainInterval[windowsIndex].push_back(endRow[i]);
        }
    }
}

vector<string> CompanyInfo::find_train_type(string inputString, char delimiter1, char delimiter2) {
    string segment;
    vector<string> seglist;
    stringstream toCut(inputString);
    char delimiter;
    if (inputString.back() == delimiter2) {
        delimiter = delimiter2;
    }
    else {
        delimiter = delimiter1;
    }
    while (getline(toCut, segment, delimiter)) {
        seglist.push_back(segment);
    }
    return seglist;
}

void CompanyInfo::ini_MATable() {
    MATable = new double *[trainDays];
    for (int i = 0; i < trainDays; i++) {
        MATable[i] = new double[257];
    }
    for (int i = 0; i < trainDays; i++) {
        for (int j = 0; j < 257; j++) {
            MATable[i][j] = 0;
        }
    }
}

void CompanyInfo::output_MA(path filePath, int MAType) {
    string type;
    switch (MAType) {
        case 0:
            type = "_SMA.csv";
            break;
        case 1:
            type = "_WMA.csv";
            break;
        case 2:
            type = "_EMA.csv";
            break;
    }

    ofstream MAOut;
    MAOut.open(filePath.stem().string() + type);
    MAOut << ",";
    for (int i = 1; i < 257; i++) {
        MAOut << i << ",";
    }
    MAOut << endl;
    for (int i = 0, dateRow = trainStartRow; i < trainDays; i++, dateRow++) {
        MAOut << date[dateRow] + ",";
        for (int j = 1; j < 257; j++) {
            MAOut << fixed << setprecision(8) << MATable[i][j] << ",";
        }
        MAOut << endl;
    }
    MAOut.close();
}

void CompanyInfo::find_cross(int high, int low) {}

CompanyInfo::~CompanyInfo() {
    delete[] date;
    delete[] price;
    for (int i = 0; i < trainDays; i++) {
        //        delete[] MATable[i];
    }
    //    delete[] MATable;
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
        //        company.cal_MA(MAType[_MAType]);
        company.train();
    }
    return 0;
}
