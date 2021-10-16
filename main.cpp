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

#define PARTICAL_AMOUNT 10
#define TOTAL_CP_LV 10000000.0
#define BUY_BIT1 7
#define BUY_BIT2 7
#define SELL_BIT1 7
#define SELL_BIT2 7

path _pricePath = "price";
int _closeCol = 4;
string _testStartYear = "2012";
string _testEndYear = "2021";
int _testYearLength = stod(_testEndYear) - stod(_testStartYear);
string _slidingWindows[] = {"YY2Y", "YH2Y", "Y2Y", "Y2H", "Y2Q", "Y2M", "H#", "H2H", "H2Q", "H2M", "Q#", "Q2Q", "Q2M", "M#", "M2M", "A2A", "10D5"};
string _slidingWindowsEX[] = {"24M12", "18M12", "12M12", "12M6", "12M3", "12M1", "6M", "6M6", "6M3", "6M1", "3M", "3M3", "3M1", "1M", "1M1", "A2A", "10D5"};
int _windowNumber = sizeof(_slidingWindows) / sizeof(_slidingWindows[0]); //暫放
int _MAUse = 0;

double _delta = 0.003;
int _expNumber = 1;
int _generationNumber = 1;

string _outputPath = "result";

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
        ~MATable() {
            delete[] date;
            for (int i = 0; i < trainDays; i++) {
                delete[] MAValues[i];
            }
            delete[] MAValues;
        }
    };
    class Window {
    public:
        string windowName_;
        string windowNameEx_;
        vector<int> interval_;
    };
    string companyName;
    string MAType;
    string *date;
    double *price;
    int totalDays;
    string MAOutputPath;
    int testStartRow;
    int testEndRow;
    int trainStartRow;
    int trainEndRow;
    int windowNumber_;
    vector<vector<int> > trainInterval;
    
    void store_date_price(path);
    string create_folder();
    void cal_MA();
    void train();
    void find_train_interval();
    void find_train_start_row(int, char);
    void find_train_start_end(vector<string>, char);
    vector<string> find_train_type(string, char &);
    vector<Window> create_sliding_windows_classes();
    void print_train();
    MATable create_MATable();
    void outputMATable();
    void find_cross(int, int);
    CompanyInfo(path filePath, string MAUse) {
        companyName = filePath.stem().string();
        store_date_price(filePath);
        MAType = MAUse;
        MAOutputPath = create_folder();
        windowNumber_ = sizeof(_slidingWindows) / sizeof(_slidingWindows[0]);
    }
    ~CompanyInfo() {
        delete[] date;
        delete[] price;
    }
};

class MAtrade {
public:
    MAtrade(int trainStartRow, int trainEndRow, CompanyInfo::MATable &table) {
        
    }
};

class MAParticle {
public:
    int buy1_bi[BUY_BIT1]{0};
    int buy2_bi[BUY_BIT2]{0};
    int sell1_bi[SELL_BIT1]{0};
    int sell2_bi[SELL_BIT2]{0};
    int buy1_dec{0};
    int buy2_dec{0};
    int sell1_dec{0};
    int sell2_dec{0};
    double rate_of_return{0};
    
    void initialize_particle();
    MAParticle() {
    }
    ~MAParticle() {
    }
};

void MAParticle::initialize_particle() {
    for (int i = 0; i < BUY_BIT1; i++) {
        buy1_bi[i] = 0;
    }
    for (int i = 0; i < BUY_BIT2; i++) {
        buy2_bi[i] = 0;
    }
    for (int i = 0; i < SELL_BIT1; i++) {
        sell1_bi[i] = 0;
    }
    for (int i = 0; i < SELL_BIT2; i++) {
        sell1_bi[i] = 0;
    }
    buy1_dec = 0;
    buy2_dec = 0;
    sell1_dec = 0;
    sell2_dec = 0;
}

class BetaMatrix {
public:
    double buy1[BUY_BIT1];
    double buy2[BUY_BIT2];
    double sell1[SELL_BIT1];
    double sell2[SELL_BIT2];
    
    BetaMatrix() {
        fill_n(buy1, BUY_BIT1, 0.5);
        fill_n(buy2, BUY_BIT2, 0.5);
        fill_n(sell1, SELL_BIT1, 0.5);
        fill_n(sell2, SELL_BIT2, 0.5);
    }
    ~BetaMatrix() {
    }
};

class GNQTS {
public:
    MAParticle particles_[PARTICAL_AMOUNT];
    BetaMatrix betaMtrix_;
    int generation_;
    
    void measure(MAParticle &);
    void convert_bi_to_dec(MAParticle &);
    void update_local();
    void update_global();
    void print_particle(MAParticle);
    void print_betaMatrix();
    GNQTS(int trainStartRow, int trainEndRow, CompanyInfo::MATable &table) : generation_(_generationNumber) {
        for (int generation = 0; generation < generation_; generation++) {
            for (int i = 0; i < PARTICAL_AMOUNT; i++) {
                measure(particles_[i]);
                convert_bi_to_dec(particles_[i]);
                MAtrade trade(trainStartRow, trainEndRow, table);
                update_local();
            }
            update_global();
        }
    }
    GNQTS() {
        
    }
    ~GNQTS() {
    }
};

void GNQTS::measure(MAParticle &particle) {
    double r;
    for (int i = 0; i < BUY_BIT1; i++) {
        r = rand();
        r = r / (double)RAND_MAX;
        if (r < betaMtrix_.buy1[i]) {
            particle.buy1_bi[i] = 1;
        }
        else {
            particle.buy1_bi[i] = 0;
        }
    }
    for (int i = 0; i < BUY_BIT2; i++) {
        r = rand();
        r = r / (double)RAND_MAX;
        if (r < betaMtrix_.buy2[i]) {
            particle.buy2_bi[i] = 1;
        }
        else {
            particle.buy2_bi[i] = 0;
        }
    }
    for (int i = 0; i < SELL_BIT1; i++) {
        r = rand();
        r = r / (double)RAND_MAX;
        if (r < betaMtrix_.sell1[i]) {
            particle.sell1_bi[i] = 1;
        }
        else {
            particle.sell1_bi[i] = 0;
        }
    }
    for (int i = 0; i < SELL_BIT2; i++) {
        r = rand();
        r = r / (double)RAND_MAX;
        if (r < betaMtrix_.sell2[i]) {
            particle.sell2_bi[i] = 1;
        }
        else {
            particle.sell2_bi[i] = 0;
        }
    }
}

void GNQTS::convert_bi_to_dec(MAParticle &particle) {
    for (int i = 0, j = BUY_BIT1 - 1; i < BUY_BIT1; i++, j--) {
        particle.buy1_dec += pow(2, j) * particle.buy1_bi[i];
    }
    for (int i = 0, j = BUY_BIT2 - 1; i < BUY_BIT2; i++, j--) {
        particle.buy2_dec += pow(2, j) * particle.buy2_bi[i];
    }
    for (int i = 0, j = SELL_BIT1 - 1; i < SELL_BIT1; i++, j--) {
        particle.sell1_dec += pow(2, j) * particle.sell1_bi[i];
    }
    for (int i = 0, j = SELL_BIT2 - 1; i < SELL_BIT2; i++, j--) {
        particle.sell2_dec += pow(2, j) * particle.sell2_bi[i];
    }
}

void GNQTS::update_local() {
}

void GNQTS::update_global() {
}

void GNQTS::print_particle(MAParticle particle) {
    for (int j = 0; j < BUY_BIT1; j++) {
        cout << particle.buy1_bi[j] << ",";
    }
    cout << "|";
    for (int j = 0; j < BUY_BIT2; j++) {
        cout << particle.buy2_bi[j] << ",";
    }
    cout << "|";
    for (int j = 0; j < SELL_BIT1; j++) {
        cout << particle.sell1_bi[j] << ",";
    }
    cout << "|";
    for (int j = 0; j < SELL_BIT2; j++) {
        cout << particle.sell2_bi[j] << ",";
    }
    cout << endl;
    cout << particle.buy1_dec << ",";
    cout << particle.buy2_dec << ",";
    cout << particle.sell1_dec << ",";
    cout << particle.sell2_dec << endl;
}

void GNQTS::print_betaMatrix() {
    for (int i = 0; i < BUY_BIT1; i++) {
        cout << betaMtrix_.buy1[i] << ",";
    }
    cout << "|";
    for (int i = 0; i < BUY_BIT2; i++) {
        cout << betaMtrix_.buy2[i] << ",";
    }
    cout << "|";
    for (int i = 0; i < SELL_BIT1; i++) {
        cout << betaMtrix_.sell1[i] << ",";
    }
    cout << "|";
    for (int i = 0; i < SELL_BIT2; i++) {
        cout << betaMtrix_.sell1[i] << ",";
    }
    cout << endl;
}

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
    find_train_interval();
    vector<Window> windowsInfo = create_sliding_windows_classes();
    MATable table = create_MATable();
    for (int i = 0; i < trainInterval.size(); i++) {
        for (int j = 0; j < trainInterval[i].size(); j++) {
            trainInterval[i][j] -= trainStartRow;
        }
    }
    for (int windowIndex = 0; windowIndex < windowNumber_; windowIndex++) {
        srand(343);
        cout << _slidingWindows[windowIndex] << endl;
        for (int intervalIndex = 0; intervalIndex < trainInterval[windowIndex].size(); intervalIndex += 2) {
            cout << table.date[trainInterval[windowIndex][intervalIndex]] + "~" + table.date[trainInterval[windowIndex][intervalIndex + 1]] << endl;
            for (int expCnt = 0; expCnt < _expNumber; expCnt++) {
//                cout << "exp:" << expCnt << endl;
                GNQTS runGNQTS(trainInterval[windowIndex][intervalIndex], trainInterval[windowIndex][intervalIndex + 1], table);
            }
        }
    }
}

void CompanyInfo::find_train_interval() {
    for (int windowsIndex = 0; windowsIndex < windowNumber_; windowsIndex++) {
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

vector<CompanyInfo::Window> CompanyInfo::create_sliding_windows_classes() {
    vector<Window> windowsInfo(windowNumber_);
    for (int windowIndex = 0; windowIndex < windowNumber_; windowIndex++) {
        windowsInfo[windowIndex].windowName_ = _slidingWindows[windowIndex];
        windowsInfo[windowIndex].windowNameEx_ = _slidingWindowsEX[windowIndex];
        windowsInfo[windowIndex].interval_ = trainInterval[windowIndex];
    }
    return windowsInfo;
}

void CompanyInfo::print_train() {
    for (int windowsIndex = 0; windowsIndex < windowNumber_; windowsIndex++) {
        cout << _slidingWindows[windowsIndex] << endl;
        for (int i = 0; i < trainInterval[windowsIndex].size(); i += 2) {
            cout << date[trainInterval[windowsIndex][i]] << "~" << date[trainInterval[windowsIndex][i + 1]] << endl;
        }
    }
}

CompanyInfo::MATable CompanyInfo::create_MATable() {
    int longestTrainMonth = 0;
    for (int i = 0; i < windowNumber_; i++) {
        char delimiter;
        string trainMonth = find_train_type(_slidingWindowsEX[i], delimiter)[0];
        if (delimiter == 'M' && stoi(trainMonth) > longestTrainMonth) {
            longestTrainMonth = stoi(trainMonth);
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
    table.MAValues = new double *[table.trainDays];
    for (int i = 0; i < table.trainDays; i++) {
        table.MAValues[i] = new double[257];
    }
    vector<path> MAFilePath = get_path(MAType + "/" + companyName);
    for (int i = 0; i < MAFilePath.size(); i++) {
        vector<vector<string> > MAFile = read_data(MAFilePath[i]);
        if (int(MAFile.size()) - table.trainDays < 0) {
            cout << companyName + " MA file not old enougth" << endl;
            exit(1);
        }
        for (int j = 0, k = int(MAFile.size()) - table.trainDays; k < MAFile.size(); j++, k++) {
            table.MAValues[j][i + 1] = stod(MAFile[k][1]);
        }
    }
    return table;
}

void CompanyInfo::outputMATable() {
    MATable table = create_MATable();
    ofstream out;
    out.open(companyName + "_" + MAType + "Table.csv");
    for (int i = 0; i < table.trainDays; i++) {
        out << table.date[i] + ",";
        for (int j = 1; j < 257; j++) {
            out << table.MAValues[i][j] << ",";
        }
        out << endl;
    }
    out.close();
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
    string MAUse[] = {"SMA", "WMA", "EMA"};
    vector<path> companyPricePath = get_path(_pricePath);
    for (int companyIndex = 0; companyIndex < 1; companyIndex++) {
        CompanyInfo company(companyPricePath[companyIndex], MAUse[_MAUse]);
        //        company.cal_MA();
//        company.train();
        company.find_train_interval();
        vector<CompanyInfo::Window> windowsInfo = company.create_sliding_windows_classes();
        for (auto i : windowsInfo) {
            cout << i.windowName_ << endl;
            cout << i.windowNameEx_ << endl;
            for (int j = 0; j < i.interval_.size(); j += 2) {
                cout << company.date[i.interval_[j]] << "~" <<company.date[i.interval_[j + 1]] << endl;
            }
        }
    }
    return 0;
}
