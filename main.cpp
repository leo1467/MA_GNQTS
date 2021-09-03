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
string _testStartYear = "2011";
string _testEndYear = "2021";

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
    void ini_MATable(int);
    void cal_MA(string);
    void output_MA(path, int);
    void find_train_interval();
    void find_window_start_end(int, int, string);
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
        } else {
            price[i - 1] = stod(priceFile[i][_closeCol]);
        }
        if (j == 0 && date[i - 1].substr(0, 4) == _testStartYear) {
            testStartRow = i - 1;
            j++;
        } else if (j == 1 && date[i - 1].substr(0, 4) == _testEndYear) {
            testEndRow = i - 2;
            j++;
        }
    }
}

void CompanyInfo::ini_MATable(int MAtype) {
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

// void CompanyInfo::find_train_interval() {
//    vector< string > slidingWindows{"A2A", "Y2Y", "Y2H", "Y2Q", "Y2M", "H#",
//    "H2H", "H2Q", "H2M", "Q#", "Q2Q", "Q2M", "M#", "M2M"}; for (int
//    windowInedx = 0; windowInedx < slidingWindows.size(); windowInedx++) {
//        if (slidingWindows[windowInedx] == "A2A") { //直接給A2A的起始與結束row
//            trainInterval[windowInedx].push_back(0);
//            trainInterval[windowInedx].push_back(totalDays - 1);
//        }
//        else {  //開始找普通滑動視窗的起始與結束
//            find_window_start_end(windowInedx,
//            (int)slidingWindows[windowInedx].size(),
//            slidingWindows[windowInedx]);
//        }
//    }
//}

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
    copy(directory_iterator(targetPath), directory_iterator(),
         back_inserter(filePath));
    sort(filePath.begin(), filePath.end());
    return filePath;
}

int main(int argc, const char *argv[]) {
    string MAType[] = {"SMA", "WMA", "EMA"};
    vector<path> companyPricePath = get_path(_pricePath);
    for (int companyIndex = 0; companyIndex < companyPricePath.size();
         companyIndex++) {
        CompanyInfo company(companyPricePath[companyIndex], MAType[_MAType]);
//        company.cal_MA(MAType[_MAType]);
    }
    return 0;
}
