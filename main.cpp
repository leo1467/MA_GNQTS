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
string _trainStartYear = "2010";
string _trainEndYear = "2021";
string _testStartYear = to_string(stoi(_trainStartYear.substr(0, 4)) + 1);

int _MAType = 0;

vector< vector< string > > read_data(path);
vector< path > get_path(path);

class CompanyInfo {
   public:
    string *date;
    double *price;
    int totalDays;
    int trainStartRow;
    int trainEndRow;
    int trainDays;
    double **MAValues;
    vector< vector< int > > trainInterval;

    void store_date_price(path);
    void ini_MAValues(int);
    void cal_MA(int);
    void output_MA(path, int);
    void find_train_interval();
    void find_window_start_end(int, int, string);
    void find_cross(int, int);
    CompanyInfo(path filePath, int MAType) {
        store_date_price(filePath);
        ini_MAValues(MAType);
    }
    ~CompanyInfo();
};

void CompanyInfo::store_date_price(path priceFilePath) {
    vector< vector< string > > priceFile = read_data(priceFilePath);
    totalDays = (int)priceFile.size();
    date = new string[totalDays];
    price = new double[totalDays];
    for (int i = 1, j = 0; i < totalDays; i++) {
        date[i] = priceFile[i][0];
        if (priceFile[i][_closeCol] == "null") {
            price[i] = 0;
        }
        else {
            price[i] = stod(priceFile[i][_closeCol]);
        }
        if (j == 0 && date[i].substr(0, 4) == _trainStartYear) {
            trainStartRow = i;
            j++;
        }
        else if (j == 1 && date[i].substr(0, 4) == _trainEndYear) {
            trainEndRow = i - 1;
            j++;
        }
    }
    trainDays = trainEndRow - trainStartRow + 1;
}

void CompanyInfo::ini_MAValues(int MAtype) {
    MAValues = new double *[trainDays];
    for (int i = 0; i < trainDays; i++) {
        MAValues[i] = new double[257];
    }
    for (int i = 0; i < trainDays; i++) {
        for (int j = 0; j < 257; j++) {
            MAValues[i][j] = 0;
        }
    }
    cal_MA(_MAType);
}

void CompanyInfo::cal_MA(int MAType) {
    switch (MAType) {
        case 0:
            for (int MA = 1; MA < 257; MA++) {
                for (int dateRow = trainStartRow, MARow = 0; dateRow <= trainEndRow; dateRow++, MARow++) {
                    double MARangePriceSum = 0;
                    for (int i = 0, j = dateRow; i < MA; i++, j--) {
                        MARangePriceSum += price[j];
                    }
                    MAValues[MARow][MA] = MARangePriceSum / MA;
                }
            }
            break;
        case 1:
            for (int MA = 1; MA < 257; MA++) {
                for (int dateRow = trainStartRow, MARow = 0; dateRow <= trainEndRow; dateRow++, MARow++) {
                    double MARangePriceSum = 0;
                    int MASum = 0;
                    int MAWeight = MA;
                    for (int MADays = 0, priceRow = dateRow; MADays < MA; MADays++, priceRow--, MASum += MAWeight, MAWeight--) {
                        MARangePriceSum += price[priceRow] * MAWeight;
                    }
                    MAValues[MARow][MA] = MARangePriceSum / MASum;
                }
            }
            break;
        case 2:
            break;
    }
}

void CompanyInfo::output_MA(path filePath, int MAType) {
    ofstream MAOut;
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
    MAOut.open(filePath.stem().string() + type);
    MAOut << ",";
    for (int i = 1; i < 257; i++) {
        MAOut << i << ",";
    }
    MAOut << endl;
    for (int i = 0, dateRow = trainStartRow; i < trainDays; i++, dateRow++) {
        MAOut << date[dateRow] + ",";
        for (int j = 1; j < 257; j++) {
            MAOut << fixed << setprecision(8) << MAValues[i][j] << ",";
        }
        MAOut << endl;
    }
    MAOut.close();
}

void CompanyInfo::find_train_interval() {
    vector< string > slidingWindows{"A2A", "Y2Y", "Y2H", "Y2Q", "Y2M", "H#", "H2H", "H2Q", "H2M", "Q#", "Q2Q", "Q2M", "M#", "M2M"};
    for (int windowInedx = 0; windowInedx < slidingWindows.size(); windowInedx++) {
        if (slidingWindows[windowInedx] == "A2A") {  //直接給A2A的起始與結束row
            trainInterval[windowInedx].push_back(0);
            trainInterval[windowInedx].push_back(totalDays - 1);
        }
        else {  //開始找普通滑動視窗的起始與結束
            find_window_start_end(windowInedx, (int)slidingWindows[windowInedx].size(), slidingWindows[windowInedx]);
        }
    }
}

void CompanyInfo::find_window_start_end(int windowIndex, int windowLen, string window) {
    string M[] = {"01", "02", "03", "04", "05", "06", "07", "08", "09", "10", "11", "12"};
    int testStartRow = 0;  //記錄測試期開始row
    for (int i = 230; i < 300; i++) {  //找出測試期開始的row
        if (date[i].substr(0, 4) == _testStartYear) {
            testStartRow = i;
            break;
        }
    }
    int trainStartRow = 0;  //記錄訓練期開始的row
    int trainEndRow = 0;  //記錄訓練期結束的row
    if (windowLen == 2) {  //判斷year-on-year
        trainStartRow = 0;
        string train_end_y = to_string(stoi(_train_end_date.substr(0, 4)) - 1);
        for (int i = totalDays - 230; i > 0; i--) {
            if (daysTable[i].substr(0, 4) == train_end_y) {
                trainEndRow = i;
                break;
            }
        }
        switch (window[0]) {  //做H*
            case 'H': {
                interval_table.push_back(train_start_row);
                string end_H = M[6];
                for (int i = 97, j = 6; i < trainEndRow; i++) {
                    if (daysTable[i].substr(5, 2) == end_H) {
                        interval_table.push_back(i - 1);
                        interval_table.push_back(i);
                        end_H = M[(j += 6) % 12];
                        i += 97;
                    }
                }
                interval_table.push_back(train_end_row);
                break;
            }
            case 'Q': {  //做Q*
                interval_table.push_back(train_start_row);
                string end_Q = M[3];
                for (int i = 55, j = 3; i < trainEndRow; i++) {
                    if (daysTable[i].substr(5, 2) == end_Q) {
                        interval_table.push_back(i - 1);
                        interval_table.push_back(i);
                        end_Q = M[(j += 3) % 12];
                        i += 55;
                    }
                }
                interval_table.push_back(train_end_row);
                break;
            }
            case 'M': {  //做M*
                interval_table.push_back(train_start_row);
                string end_H = M[1];
                for (int i = 15, j = 1; i < trainEndRow; i++) {
                    if (daysTable[i].substr(5, 2) == end_H) {
                        interval_table.push_back(i - 1);
                        interval_table.push_back(i);
                        end_H = M[++j % 12];
                        i += 15;
                    }
                }
                interval_table.push_back(train_end_row);
                break;
            }
        }
    }
    else {
        string mon = _train_start_date.substr(5, 2);  //記錄目前iterate到什麼月份
        string start;  //記錄sliding window開始月份
        string end;  //記錄sliding window結束月份
        int s_jump_e = 0;  //從period頭要找period尾
        int e_jump_s = 0;  //從period尾要找下一個period頭
        int skip = 0;  //M[]要跳幾個月份
        switch (windowUse[0]) {  //看是什麼開頭，找出第一個訓練開始的row以及開始月份及結束月份
            case 'Y': {
                s_jump_e = find_train_start(test_s_row, mon, train_start_row, start, end, 13, daysTable) - 1;
                break;
            }
            case 'H': {
                s_jump_e = find_train_start(test_s_row, mon, train_start_row, start, end, 7, daysTable) - 1;
                break;
            }
            case 'Q': {
                s_jump_e = find_train_start(test_s_row, mon, train_start_row, start, end, 4, daysTable) - 1;
                break;
            }
            case 'M': {
                s_jump_e = find_train_start(test_s_row, mon, train_start_row, start, end, 2, daysTable) - 1;
                break;
            }
        }
        switch (windowUse[2]) {  //看是什麼結尾，找出最後一個訓練開始的row
            case 'Y': {
                skip = find_train_end(table_size, mon, train_end_row, 13, daysTable) - 1;
                break;
            }
            case 'H': {
                skip = find_train_end(table_size, mon, train_end_row, 7, daysTable) - 1;
                break;
            }
            case 'Q': {
                skip = find_train_end(table_size, mon, train_end_row, 4, daysTable) - 1;
                break;
            }
            case 'M': {
                skip = find_train_end(table_size, mon, train_end_row, 2, daysTable) - 1;
                break;
            }
        }
        int train_s_M = 0, train_e_M = 12;  //記錄第一個訓練期開頭及結尾在M[]的index
        for (int i = 0; i < 12; i++) {
            if (start == M[i]) {
                train_s_M = i;
                break;
            }
        }
        if (windowUse[0] == windowUse[2]) {  //判斷從period尾要找下一個period頭需要跳多少
            e_jump_s = 1;
        }
        else if (windowUse[2] == 'M') {
            e_jump_s = s_jump_e - 1;
        }
        else if (windowUse[2] == 'Q') {
            e_jump_s = s_jump_e - 3;
        }
        else {
            e_jump_s = 6;
        }
        interval_table.push_back(train_start_row);
        for (int i = trainStartRow + s_jump_e * 18, k = 1; i < trainEndRow; i++) {
            if (k % 2 == 1 && daysTable[i].substr(5, 2) == end) {
                interval_table.push_back(i - 1);
                start = M[(train_s_M += skip) % 12];
                end = M[(train_e_M += skip) % 12];
                i -= e_jump_s * 24;
                if (i < 0) {
                    i = 0;
                }
                k++;
            }
            else if (k % 2 == 0 && daysTable[i].substr(5, 2) == start) {
                interval_table.push_back(i);
                i += s_jump_e * 18;
                k++;
            }
        }
        interval_table.push_back(train_end_row);
    }
}

void CompanyInfo::find_cross(int high, int low) {
}

CompanyInfo::~CompanyInfo() {
    delete[] date;
    delete[] price;
    for (int i = 0; i < trainDays; i++) {
        delete[] MAValues[i];
    }
    delete[] MAValues;
}

vector< vector< string > > read_data(path filePath) {
    ifstream infile(filePath);
    vector< vector< string > > data;
    if (!infile) {
        cout << filePath.string() + " not found" << endl;
        exit(1);
    }
    cout << "reading " + filePath.filename().string() << endl;
    string row;
    string cell;
    vector< string > oneRow;
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
        vector< string >().swap(oneRow);
    }
    infile.close();
    return data;
}

vector< path > get_path(path targetPath) {
    vector< path > filePath;
    copy(directory_iterator(targetPath), directory_iterator(), back_inserter(filePath));
    sort(filePath.begin(), filePath.end());
    return filePath;
}

int main(int argc, const char *argv[]) {
    vector< path > companyPricePath = get_path(_pricePath);
    for (int companyIndex = 0; companyIndex < 1; companyIndex++) {
        // if (companyPricePath[i].stem() == "V") {
        CompanyInfo company(companyPricePath[companyIndex], _MAType);
        // company.output_MA(companyPricePath[i], _MAType);
        // }
    }
    return 0;
}
