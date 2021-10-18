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

int _techIndex = 0;
int _MAUse = 0;

double _delta = 0.003;
int _expNumber = 1;
int _generationNumber = 1;

string _outputPath = "result";

vector<vector<string> > read_data(path);
vector<path> get_path(path);
vector<string> find_train_type(string, char &);

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
    class TrainWindow {
    public:
        string windowName_;
        string windowNameEx_;
        vector<int> interval_;
        int trainStartRow_{-1};
        int trainEndRow_{-1};
        CompanyInfo &company_;
        
        void find_train_interval();
        void find_train_start_row(int, char);
        void find_train_start_end(vector<string>, char);
        void print_train();
        TrainWindow(string window, string windowEx, CompanyInfo &company) : windowName_(window), windowNameEx_(windowEx), company_(company) {
            find_train_interval();
            for (int &i : interval_) {
                i -= company_.longestTrainRow_;
            }
        }
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
    int longestTrainMonth_{0};
    int longestTrainRow_;
    int windowNumber_;
    
    void store_date_price(path);
    string create_folder();
    void cal_MA();
    void train(CompanyInfo &);
    void find_train_start_row(int, char);
    void print_train();
    void find_longest_train_month_row();
    MATable create_MATable();
    void outputMATable();
    CompanyInfo(path filePath, string MAUse) {
        companyName = filePath.stem().string();
        store_date_price(filePath);
        MAType = MAUse;
        MAOutputPath = create_folder();
        windowNumber_ = sizeof(_slidingWindows) / sizeof(_slidingWindows[0]);
        find_longest_train_month_row();
        find_train_start_row(longestTrainMonth_, 'M');
    }
    ~CompanyInfo() {
        delete[] date;
        delete[] price;
    }
};

class MA_GNQTS {
public:
    class Trade {
    public:
        Trade(int trainStartRow, int trainEndRow, CompanyInfo::MATable &table) {
            
        }
    };
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
    class Particle {
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
        Particle() {
        }
        ~Particle() {
        }
    };
    Particle particles_[PARTICAL_AMOUNT];
    BetaMatrix betaMtrix_;
    int generation_;
    
    void measure(Particle &);
    void convert_bi_to_dec(Particle &);
    void update_local();
    void update_global();
    void print_particle(Particle);
    void print_betaMatrix();
    MA_GNQTS(int trainStartRow, int trainEndRow, CompanyInfo::MATable &table) : generation_(_generationNumber) {
        for (int generation = 0; generation < generation_; generation++) {
            for (int i = 0; i < PARTICAL_AMOUNT; i++) {
                measure(particles_[i]);
                convert_bi_to_dec(particles_[i]);
                Trade trade(trainStartRow, trainEndRow, table);
                update_local();
            }
            update_global();
        }
    }
    MA_GNQTS() {
        
    }
    ~MA_GNQTS() {
    }
};

void MA_GNQTS::Particle::initialize_particle() {
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

void MA_GNQTS::measure(Particle &particle) {
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

void MA_GNQTS::convert_bi_to_dec(Particle &particle) {
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

void MA_GNQTS::update_local() {
}

void MA_GNQTS::update_global() {
}

void MA_GNQTS::print_particle(Particle particle) {
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

void MA_GNQTS::print_betaMatrix() {
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

void CompanyInfo::TrainWindow::find_train_interval() {
    char delimiter;
    vector<string> trainType = find_train_type(windowNameEx_, delimiter);
    if (windowName_ == "A2A") {
        interval_.push_back(company_.testStartRow);
        interval_.push_back(company_.testEndRow);
    }
    else {
        find_train_start_end(trainType, delimiter);
    }
}

void CompanyInfo::TrainWindow::find_train_start_row(int trainPeriodLength, char delimiter) {
    if (delimiter == 'M') {
        for (int i = company_.testStartRow - 1, monthCount = 0; i >= 0; i--) {
            if (company_.date[i].substr(5, 2) != company_.date[i - 1].substr(5, 2)) {
                monthCount++;
                if (monthCount == trainPeriodLength) {
                    trainStartRow_ = i;
                    break;
                }
            }
        }
    }
    else if (delimiter == 'D') {
        trainStartRow_ = company_.testStartRow - trainPeriodLength;
        trainEndRow_ = company_.testStartRow - 1;
    }
    if (trainStartRow_ == -1) {
        cout << windowName_ + " can not find trainStartRow " << trainPeriodLength << endl;
        exit(1);
    }
}

void CompanyInfo::TrainWindow::find_train_start_end(vector<string> trainType, char delimiter) {
    vector<int> startRow;
    vector<int> endRow;
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
        startRow.push_back(trainStartRow_);
        for (int i = trainStartRow_, intervalCount = 1, monthCount = 0; intervalCount < intervalNum; i++) {
            if (company_.date[i].substr(5, 2) != company_.date[i + 1].substr(5, 2)) {
                monthCount++;
                if (monthCount == testPeriodLength) {
                    startRow.push_back(i + 1);
                    intervalCount++;
                    monthCount = 0;
                }
            }
        }
        if (trainType.size() == 2) {
            endRow.push_back(company_.testStartRow - 1);
            trainEndRow_ = company_.testStartRow;
        }
        else if (trainType.size() == 1) {
            for (int i = trainStartRow_, monthCount = 0; i < company_.totalDays; i++) {
                if (company_.date[i].substr(5, 2) != company_.date[i + 1].substr(5, 2)) {
                    monthCount++;
                    if (monthCount == trainPeriodLength) {
                        endRow.push_back(i);
                        trainEndRow_ = i + 1;
                        break;
                    }
                }
            }
        }
        for (int i = trainEndRow_, intervalCount = 1, monthCount = 0; intervalCount < intervalNum; i++) {
            if (company_.date[i].substr(5, 2) != company_.date[i + 1].substr(5, 2)) {
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
        for (int i = trainStartRow_; i <= company_.testEndRow - trainPeriodLength; i += testPeriodLength) {
            startRow.push_back(i);
        }
        for (int i = trainEndRow_; i < company_.testEndRow; i += testPeriodLength) {
            endRow.push_back(i);
        }
    }
    for (int i = 0; i < startRow.size(); i++) {
        interval_.push_back(startRow[i]);
        interval_.push_back(endRow[i]);
    }
    //=======================================
}

void CompanyInfo::TrainWindow::print_train() {
    cout << windowName_ + "=" + windowNameEx_ << endl;
    for (int i = 0; i < interval_.size(); i += 2) {
        cout << company_.date[interval_[i] + company_.longestTrainRow_] + "~" + company_.date[interval_[i + 1] + company_.longestTrainRow_] << endl;
    }
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

void CompanyInfo::train(CompanyInfo &company) {
    MATable table = create_MATable();
    for (int windowIndex = 0; windowIndex < windowNumber_; windowIndex++) {
        srand(343);
        TrainWindow window(_slidingWindows[windowIndex], _slidingWindowsEX[windowIndex], company);
        window.print_train();
        for (int intervalIndex = 0; intervalIndex < window.interval_.size(); intervalIndex += 2) {
            for (int expCnt = 0; expCnt < _expNumber; expCnt++) {
//                cout << "exp:" << expCnt << endl;
                MA_GNQTS runGNQTS(window.interval_[intervalIndex], window.interval_[intervalIndex + 1], table);
            }
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

void CompanyInfo::find_longest_train_month_row() {
    for (int i = 0; i < windowNumber_; i++) {
        char delimiter;
        string trainMonth = find_train_type(_slidingWindowsEX[i], delimiter)[0];
        if (delimiter == 'M' && stoi(trainMonth) > longestTrainMonth_) {
            longestTrainMonth_ = stoi(trainMonth);
        }
    }
    longestTrainRow_ = -1;
    for (int i = testStartRow - 1, monthCount = 0; i >= 0; i--) {
        if (date[i].substr(5, 2) != date[i - 1].substr(5, 2)) {
            monthCount++;
            if (monthCount == longestTrainMonth_) {
                longestTrainRow_ = i;
                break;
            }
        }
    }
}

CompanyInfo::MATable CompanyInfo::create_MATable() {
    MATable table;
    table.trainDays = totalDays - longestTrainRow_;
    table.date = new string[table.trainDays];
    table.price = new double[table.trainDays];
    for (int i = longestTrainRow_, j = 0; i < totalDays; i++, j++) {
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

vector<string> find_train_type(string window, char &delimiter) {
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

int main(int argc, const char *argv[]) {
    string MAUse[] = {"SMA", "WMA", "EMA"};
    vector<path> companyPricePath = get_path(_pricePath);
    for (int companyIndex = 0; companyIndex < 1; companyIndex++) {
        CompanyInfo company(companyPricePath[companyIndex], MAUse[_MAUse]);
        company.train(company);
    }
    return 0;
}
