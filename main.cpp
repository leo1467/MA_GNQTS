#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;
using namespace chrono;
using namespace filesystem;

#define PARTICAL_AMOUNT 10
#define TOTAL_CP_LV 10000000.0
#define BUY1_BITS 8
#define BUY2_BITS 8
#define SELL1_BITS 8
#define SELL2_BITS 8

int _mode = 10;
string _setCompany = "AAPL";
string _setWindow = "M2M";
int _MAUse = 0;
string _MA[] = {"SMA", "WMA", "EMA"};
int _algoUse = 2;
string _algo[] = {"QTS", "GQTS", "GNQTS", "KNQTS"};

double _delta = 0.00012;
int _expNumber = 50;
int _generationNumber = 10000;
double _multiplyUp = 1.01;
double _multiplyDown = 0.99;
int _testDeltaLoop = 1;  //normal is 0
double _testDeltaGap = 0.00001;

string _testStartYear = "2012";
string _testEndYear = "2021";
double _testYearLength = stod(_testEndYear) - stod(_testStartYear);
vector<string> _slidingWindows{"A2A", "YYY2YYY", "YYY2YY", "YYY2YH", "YYY2Y", "YYY2H", "YYY2Q", "YYY2M", "YY2YY", "YY2YH", "YY2Y", "YY2H", "YY2Q", "YY2M", "YH2YH", "YH2Y", "YH2H", "YH2Q", "YH2M", "Y2Y", "Y2H", "Y2Q", "Y2M", "H2H", "H2Q", "H2M", "Q2Q", "Q2M", "M2M", "H#", "Q#", "M#", "20D20", "20D15", "20D10", "20D5", "15D15", "15D10", "15D5", "10D10", "10D5", "5D5", "5D4", "5D3", "5D2", "4D3", "4D2", "3D2", "2D2", "4W4", "4W3", "4W2", "4W1", "3W3", "3W2", "3W1", "2W2", "2W1", "1W1"};
vector<string> _slidingWindowsEX{"A2A", "36M36", "36M24", "36M18", "36M12", "36M6", "36M3", "36M1", "24M24", "24M18", "24M12", "24M6", "24M3", "24M1", "18M18", "18M12", "18M6", "18M3", "18M1", "12M12", "12M6", "12M3", "12M1", "6M6", "6M3", "6M1", "3M3", "3M1", "1M1", "6M", "3M", "1M", "20D20", "20D15", "20D10", "20D5", "15D15", "15D10", "15D5", "10D10", "10D5", "5D5", "5D4", "5D3", "5D2", "4D3", "4D2", "3D2", "2D2", "4W4", "4W3", "4W2", "4W1", "3W3", "3W2", "3W1", "2W2", "2W1", "1W1"};

string _outputPath = _MA[_MAUse] + "_result";
path _pricePath = "price";  //若要讀取到小數2位的用price.2，沒有的話用price
int _closeCol = 4;

int _techIndex = 0;
int _outputDecimal2 = 0;  //產生小數2位的股價跟SMA

vector<vector<string>> read_data(path filePath) {
    ifstream infile(filePath);
    vector<vector<string>> data;
    if (!infile) {
        cout << filePath << " not found" << endl;
        exit(1);
    }
        //    cout << "reading " << filePath.filename() << endl;
    string row;
    string cell;
    vector<string> oneRow;
    while (getline(infile, row)) {
        stringstream lineStream(row);
        while (getline(lineStream, cell, ',')) {
            if (cell != "\r") {
                oneRow.push_back(cell);
            }
        }
        data.push_back(oneRow);
        row.clear();
        cell.clear();
        oneRow.clear();
    }
    infile.close();
    return data;
}

vector<path> get_path(path targetPath) {
    vector<path> filePath;
    copy(directory_iterator(targetPath), directory_iterator(), back_inserter(filePath));
    sort(filePath.begin(), filePath.end());
    for (auto i = filePath.begin(); i != filePath.end(); i++) {
        if (i->filename() == ".DS_Store") {
            filePath.erase(i);
        }
    }
    return filePath;
}

bool is_double(const string &s) {
    string::const_iterator it = s.begin();
    int dotsCnt = 0;
    while (it != s.end() && (isdigit(*it) || *it == '.')) {
        if (*it == '.') {
            ++dotsCnt;
            if (dotsCnt > 1 || dotsCnt == s.size()) {
                break;
            }
        }
        ++it;
    }
    return !s.empty() && it == s.end();
}

string set_precision(const double inputDouble, const int n = 10) {
    stringstream ss;
    ss << fixed << setprecision(n) << inputDouble;
    return ss.str();
}

class CompanyInfo {
public:
    class MATable {
    public:
        int days__;
        string *date__;
        double *price__;
        double **MAtable__;
        
        void create_MAtable(CompanyInfo &company);
        MATable(CompanyInfo &company);
        ~MATable();
    };
    class TrainWindow {
    public:
        string windowName__;
        string windowNameEx__;
        vector<int> interval__;
        int firstTrainStartRow__ = -1;
        int firstTrainEndRow__ = -1;
        CompanyInfo &company__;
        
        void find_train_interval();
        void find_first_train_start_row(int trainPeriodLength, char delimiter);
        void find_M_train(vector<string> trainType, char delimiter);
        void find_D_train(vector<string> trainType, char delimiter);
        void find_W_train(vector<string> trainType, char delimiter);
        static int cal_weekday(string date);
        static bool is_week_changed(CompanyInfo &comapny, int bigWeekDay, int smallWeekDay, int big_i, int small_i);
        static vector<string> find_train_and_test_len(string window, char &delimiter);
        static void check_startRowSize_endRowSize(int startRowSize, int endRowSize);
        void print_train();
        TrainWindow(string window, CompanyInfo &company);
    };
    class TestWindow {
    public:
        string windowName__;
        string windowNameEx__;
        vector<int> interval__;
        CompanyInfo &company__;
        
        void find_test_interval();
        void find_M_test(int testLength);
        void find_D_test(int testLength);
        void find_W_test(int testLength);
        void print_test();
        TestWindow(string window, CompanyInfo &company);
    };
    string companyName_;
    string MAType_;
    string *date_;
    double *price_;
    int totalDays_;
    string MAOutputPath_;
    int testStartRow_;
    int testEndRow_;
    int trainStartRow_;
    int trainEndRow_;
    int longestTrainMonth_ = 0;
    int longestTrainRow_ = -1;
    int windowNumber_;
    vector<vector<double>> MAtable_;
    string trainFilePath_;
    string testFilePath_;
    string trainTraditionFilePath_;
    string testTraditionFilePath_;
    
    void store_date_price(path priceFilePath);
    void create_folder();
    void store_MA_to_vector();
    void output_MA();
    void train(string targetWindow = "all", string startDate = "", string endDate = "", bool debug = false, bool record = false);
    void find_train_start_row(int trainPeriodLength, char delimiter);
    void print_train(string targetWindow = "all");
    void test(string targetWindow = "all");
    void print_test(string targetWindow = "all");
    void find_longest_train_month_row();
    void output_MATable();
    void instant_trade(string startDate, string endDate, int buy1, int buy2, int sell1, int sell2);
    CompanyInfo(path filePath, string MAUse);
    ~CompanyInfo();
};

class MA_GNQTS {
public:
    class BetaMatrix {
    public:
        vector<double> buy1__;
        vector<double> buy2__;
        vector<double> sell1__;
        vector<double> sell2__;
        
        void initilaize();
        void print(ofstream &out, bool debug);
        BetaMatrix();
    };
    class Particle {
    public:
        vector<int> buy1_bi__;
        vector<int> buy2_bi__;
        vector<int> sell1_bi__;
        vector<int> sell2_bi__;
        int buy1_dec__ = 0;
        int buy2_dec__ = 0;
        int sell1_dec__ = 0;
        int sell2_dec__ = 0;
        int buyNum__ = 0;
        int sellNum__ = 0;
        vector<string> tradeRecord__;
        double remain__ = TOTAL_CP_LV;
        double RoR__ = 0;
        bool isRecordOn__ = false;
        int gen__ = 0;
        int exp__ = 0;
        int bestCnt__ = 0;
        
        void print(ofstream &out, bool debug);
        void initialize(double RoR = 0);
        void measure(BetaMatrix &beta);
        void convert_bi_dec();
        void record_buy_info(CompanyInfo::MATable &table, int i, int stockHold);
        void record_sell_info(CompanyInfo::MATable &table, int i, int stockHold);
        void record_last_info();
        static bool check_buy_cross(int stockHold, double MAbuy1PreDay, double MAbuy2PreDay, double MAbuy1Today, double MAbuy2Today, int i, int endRow);
        static bool check_sell_cross(int stockHold, double MAsell1PreDay, double MAsell2PreDay, double MAsell1Today, double MAsell2Today, int i, int endRow);
        void trade(int startRow, int endRow, CompanyInfo::MATable &table, bool lastRecord = false);
        void print_trade_record(ofstream &out);
        string set_output_filePath(int actualEndRow, int actualStartRow, CompanyInfo &company, string &outputPath, CompanyInfo::MATable &table);
        void print_train_test_data(CompanyInfo &company_, CompanyInfo::MATable &MAtable_, string trainPath, int actualStartRow_, int actualEndRow_);
        
        Particle(int buy1 = 0, int buy2 = 0, int sell1 = 0, int sell2 = 0, bool on = false);
    };
    
    vector<Particle> particles_;
    BetaMatrix betaMtrix_;
    Particle localBest_;
    Particle localWorst_;
    Particle globalBest_;
    Particle best_;
    int actualStartRow_ = -1;
    int actualEndRow_ = -1;
    CompanyInfo::MATable &table_;
    CompanyInfo &company_;
    double delta_ = _delta;
    int compareNew_ = 0;
    int compareOld_ = 0;
    double multiplyUp_ = _multiplyUp;
    double multiplyDown_ = _multiplyDown;
    
    void compare_bits(vector<int> &bestVector, vector<int> &worstVector, int bits);
    void compare_and_multiply();
    void update_global();
    void find_new_row(string startDate, string endDate);
    void set_row_and_break_conditioin(string startDate, int &windowIndex, int &intervalIndex, CompanyInfo::TrainWindow &window);
    void GNQTS();
    void GQTS();
    void QTS();
    void print_debug_beta(bool debug, ofstream &out);
    void update_best();
    void is_record_on(bool record);
    CompanyInfo::TrainWindow set_wondow(string &startDate, string &targetWindow, int &windowIndex);
    ofstream set_debug_file(bool debug);
    void print_debug_exp(bool debug, int expCnt, ofstream &out);
    void print_debug_gen(bool debug, int generation, ofstream &out);
    void print_debug_particle(bool debug, int i, ofstream &out);
    void store_exp_gen(int expCnt, int generation);
    void start_gen(bool debug, int expCnt, int generation, ofstream &out);
    void initialize_KNQTS();
    void start_exp(bool debug, int expCnt, ofstream &out);
    
    MA_GNQTS(CompanyInfo &company, CompanyInfo::MATable &table, string targetWindow, string startDate, string endDate, bool debug, bool record);
};

class CalculateTest {
public:
    MA_GNQTS::Particle p_;
    CompanyInfo &company_;
    CompanyInfo::MATable &table_;
    
    CompanyInfo::TestWindow set_window(string &actualWindow, string &targetWindow, int &windowIndex);
    void check_exception(vector<path> &eachTrainFilePath, CompanyInfo::TestWindow &window);
    void set_test_output_path(string &testFileOutputPath, string &trainFilePath, bool tradition);
    
    CalculateTest(CompanyInfo &company, CompanyInfo::MATable &table, string targetWindow, bool tradition = false);
};

class BH {
public:
    double BHRoR;
    BH(string startDate, string endDate, CompanyInfo &company, double totalCPLv) {
        int startRow = -1;
        int endRow = -1;
        for (int i = 0; i < company.totalDays_; i++) {
            if (startDate == company.date_[i]) {
                startRow = i;
                break;
            }
        }
        for (int i = startRow; i < company.totalDays_; i++) {
            if (endDate == company.date_[i]) {
                endRow = i;
                break;
            }
        }
        if (startRow == -1 || endRow == -1) {
            cout << "cant find B&H startRow or endRow" << endl;
            exit(1);
        }
        int stockHold = totalCPLv / company.price_[startRow];
        double remain = totalCPLv - stockHold * company.price_[startRow];
        remain += stockHold * company.price_[endRow];
        BHRoR = ((remain - totalCPLv) / totalCPLv);
    }
};

class Tradition {
public:
    vector<vector<int>> traditionStrategy{{5, 10, 5, 10}, {5, 20, 5, 20}, {5, 60, 5, 60}, {10, 20, 10, 20}, {10, 60, 10, 60}, {20, 60, 20, 60}, {120, 240, 120, 240}};
    CompanyInfo &company_;
    vector<MA_GNQTS::Particle> p_;
    vector<MA_GNQTS::Particle> eachBestP_;
    
    CompanyInfo::TrainWindow set_window(string &targetWindow, int &windowIndex);
    void trainTradition(CompanyInfo::MATable &table, string targetWindow);
    
    Tradition(CompanyInfo &company, string targetWindow = "all");
};

CompanyInfo::TrainWindow Tradition::set_window(string &targetWindow, int &windowIndex) {
    string actualWindow = _slidingWindows[windowIndex];
    if (targetWindow != "all") {
        actualWindow = targetWindow;
        windowIndex = company_.windowNumber_;
    }
    CompanyInfo::TrainWindow window(actualWindow, company_);
    cout << actualWindow << endl;
    return window;
}

void Tradition::trainTradition(CompanyInfo::MATable &table, string targetWindow) {
    cout << "train " << company_.companyName_ << " tradition" << endl;
    int startRow = 0;
    int endRow = 0;
    string outputPath = "";
    for (int windowIndex = 0; windowIndex < company_.windowNumber_; windowIndex++) {
        CompanyInfo::TrainWindow window = set_window(targetWindow, windowIndex);
        for (int intervalIndex = 0; intervalIndex < window.interval__.size(); intervalIndex += 2) {
            startRow = window.interval__[intervalIndex];
            endRow = window.interval__[intervalIndex + 1];
            outputPath = company_.trainTraditionFilePath_ + window.windowName__;
            for (int i = 0; i < p_.size(); i++) {
                p_[i].initialize();
                p_[i].buy1_dec__ = traditionStrategy[i][0];
                p_[i].buy2_dec__ = traditionStrategy[i][1];
                p_[i].sell1_dec__ = traditionStrategy[i][2];
                p_[i].sell2_dec__ = traditionStrategy[i][3];
                p_[i].trade(startRow, endRow, table);
            }
            stable_sort(p_.begin(), p_.end(), [](const MA_GNQTS::Particle &a, const MA_GNQTS::Particle &b) { return a.RoR__ > b.RoR__; });
                //                eachBestP_.push_back(p_[0]);
            p_[0].print_train_test_data(company_, table, outputPath, startRow, endRow);
        }
    }
}

Tradition::Tradition(CompanyInfo &company, string targetWindow) : company_(company), p_(traditionStrategy.size()) {
    CompanyInfo::MATable table(company_);
    trainTradition(table, targetWindow);
    CalculateTest runTest(company_, table, targetWindow, true);
}

CompanyInfo::TestWindow CalculateTest::set_window(string &actualWindow, string &targetWindow, int &windowIndex) {
    if (targetWindow != "all") {
        actualWindow = targetWindow;
        windowIndex = company_.windowNumber_;
    }
    CompanyInfo::TestWindow window(actualWindow, company_);
    cout << window.windowName__ << endl;
    return window;
}

void CalculateTest::check_exception(vector<path> &eachTrainFilePath, CompanyInfo::TestWindow &window) {
    if (eachTrainFilePath.size() != window.interval__.size() / 2) {
        cout << window.windowName__ << " test interval number is not equal to train fle number" << endl;
        exit(1);
    }
}

void CalculateTest::set_test_output_path(string &testFileOutputPath, string &trainFilePath, bool tradition) {
    trainFilePath = company_.trainFilePath_;
    testFileOutputPath = company_.testFilePath_;
    if (tradition) {
        trainFilePath = company_.trainTraditionFilePath_;
        testFileOutputPath = company_.testTraditionFilePath_;
    }
    cout << "test " << company_.companyName_ << endl;
}

CalculateTest::CalculateTest(CompanyInfo &company, CompanyInfo::MATable &table, string targetWindow, bool tradition) : company_(company), table_(table) {
    string trainFilePath;
    string testFileOutputPath;
    set_test_output_path(testFileOutputPath, trainFilePath, tradition);
    for (int windowIndex = 0; windowIndex < company_.windowNumber_; windowIndex++) {
        string actualWindow = _slidingWindows[windowIndex];
        if (actualWindow != "A2A") {
            CompanyInfo::TestWindow window = set_window(actualWindow, targetWindow, windowIndex);
            vector<path> eachTrainFilePath = get_path(trainFilePath + window.windowName__);
            testFileOutputPath += window.windowName__;
            for (int intervalIndex = 0, trainFileIndex = 0; intervalIndex < window.interval__.size(); intervalIndex += 2, trainFileIndex++) {
                check_exception(eachTrainFilePath, window);
                vector<vector<string>> thisTrainFile = read_data(eachTrainFilePath[trainFileIndex]);
                p_.initialize();
                p_.buy1_dec__ = stoi(thisTrainFile[10][1]);
                p_.buy2_dec__ = stoi(thisTrainFile[11][1]);
                p_.sell1_dec__ = stoi(thisTrainFile[12][1]);
                p_.sell2_dec__ = stoi(thisTrainFile[13][1]);
                p_.print_train_test_data(company_, table_, testFileOutputPath, window.interval__[intervalIndex], window.interval__[intervalIndex + 1]);
            }
        }
    }
}

void MA_GNQTS::Particle::initialize(double RoR) {
    fill(buy1_bi__.begin(), buy1_bi__.end(), 0);
    fill(buy2_bi__.begin(), buy2_bi__.end(), 0);
    fill(sell1_bi__.begin(), sell1_bi__.end(), 0);
    fill(sell2_bi__.begin(), sell2_bi__.end(), 0);
    buy1_dec__ = 0;
    buy2_dec__ = 0;
    sell1_dec__ = 0;
    sell2_dec__ = 0;
    buyNum__ = 0;
    sellNum__ = 0;
    remain__ = TOTAL_CP_LV;
    RoR__ = RoR;
    tradeRecord__.clear();
    gen__ = 0;
    exp__ = 0;
    bestCnt__ = 0;
    isRecordOn__ = false;
}

void MA_GNQTS::Particle::measure(BetaMatrix &beta) {
    double r;
    for (int i = 0; i < BUY1_BITS; i++) {
        r = (double)rand() / (double)RAND_MAX;
        if (r < beta.buy1__[i]) {
            buy1_bi__[i] = 1;
        }
        else {
            buy1_bi__[i] = 0;
        }
    }
    for (int i = 0; i < BUY2_BITS; i++) {
        r = (double)rand() / (double)RAND_MAX;
        if (r < beta.buy2__[i]) {
            buy2_bi__[i] = 1;
        }
        else {
            buy2_bi__[i] = 0;
        }
    }
    for (int i = 0; i < SELL1_BITS; i++) {
        r = (double)rand() / (double)RAND_MAX;
        if (r < beta.sell1__[i]) {
            sell1_bi__[i] = 1;
        }
        else {
            sell1_bi__[i] = 0;
        }
    }
    for (int i = 0; i < SELL2_BITS; i++) {
        r = (double)rand() / (double)RAND_MAX;
        if (r < beta.sell2__[i]) {
            sell2_bi__[i] = 1;
        }
        else {
            sell2_bi__[i] = 0;
        }
    }
}

void MA_GNQTS::Particle::convert_bi_dec() {
    for (int i = 0, j = BUY1_BITS - 1; i < BUY1_BITS; i++, j--) {
        buy1_dec__ += pow(2, j) * buy1_bi__[i];
    }
    buy1_dec__++;
    for (int i = 0, j = BUY2_BITS - 1; i < BUY2_BITS; i++, j--) {
        buy2_dec__ += pow(2, j) * buy2_bi__[i];
    }
    buy2_dec__++;
    for (int i = 0, j = SELL1_BITS - 1; i < SELL1_BITS; i++, j--) {
        sell1_dec__ += pow(2, j) * sell1_bi__[i];
    }
    sell1_dec__++;
    for (int i = 0, j = SELL2_BITS - 1; i < SELL2_BITS; i++, j--) {
        sell2_dec__ += pow(2, j) * sell2_bi__[i];
    }
    sell2_dec__++;
}

void MA_GNQTS::Particle::record_buy_info(CompanyInfo::MATable &table, int i, int stockHold) {
    tradeRecord__.push_back("buy," + table.date__[i] + "," + set_precision(table.price__[i]) + "," + set_precision(table.MAtable__[i - 1][buy1_dec__]) + "," + set_precision(table.MAtable__[i - 1][buy2_dec__]) + "," + set_precision(table.MAtable__[i][buy1_dec__]) + "," + set_precision(table.MAtable__[i][buy2_dec__]) + "," + to_string(stockHold) + "," + set_precision(remain__) + "," + set_precision(remain__ + stockHold * table.price__[i]) + "\r");
}

void MA_GNQTS::Particle::record_sell_info(CompanyInfo::MATable &table, int i, int stockHold) {
    tradeRecord__.push_back("sell," + table.date__[i] + "," + set_precision(table.price__[i]) + "," + set_precision(table.MAtable__[i - 1][sell1_dec__]) + "," + set_precision(table.MAtable__[i - 1][sell2_dec__]) + "," + set_precision(table.MAtable__[i][sell1_dec__]) + "," + set_precision(table.MAtable__[i][sell2_dec__]) + "," + to_string(stockHold) + "," + set_precision(remain__) + "," + set_precision(remain__ + stockHold * table.price__[i]) + "\r\r");
}

void MA_GNQTS::Particle::record_last_info() {
    tradeRecord__.push_back("buyNum," + to_string(buyNum__) + ",sellNum," + to_string(sellNum__) + "\rremain," + set_precision(remain__) + "\rreturn rate," + set_precision(RoR__) + "%\r");
}

bool MA_GNQTS::Particle::check_buy_cross(int stockHold, double MAbuy1PreDay, double MAbuy2PreDay, double MAbuy1Today, double MAbuy2Today, int i, int endRow) {
    return stockHold == 0 && MAbuy1PreDay <= MAbuy2PreDay && MAbuy1Today > MAbuy2Today && i != endRow;
}

bool MA_GNQTS::Particle::check_sell_cross(int stockHold, double MAsell1PreDay, double MAsell2PreDay, double MAsell1Today, double MAsell2Today, int i, int endRow) {
    return stockHold != 0 && ((MAsell1PreDay >= MAsell2PreDay && MAsell1Today < MAsell2Today) || i == endRow);
}

void MA_GNQTS::Particle::trade(int startRow, int endRow, CompanyInfo::MATable &table, bool lastRecord) {
    int stockHold{0};
    if (isRecordOn__) {
        tradeRecord__.push_back(",date,price,preday 1,preday 2,today 1,today 2,stockHold,remain,capital lv\r");
    }
    if (buyNum__ != 0 || sellNum__ != 0) {
        buyNum__ = 0;
        sellNum__ = 0;
    }
    for (int i = startRow; i <= endRow; i++) {
        if (check_buy_cross(stockHold, table.MAtable__[i - 1][buy1_dec__], table.MAtable__[i - 1][buy2_dec__], table.MAtable__[i][buy1_dec__], table.MAtable__[i][buy2_dec__], i, endRow) && remain__ >= table.price__[i]) {
            stockHold = floor(remain__ / table.price__[i]);
            if (_pricePath == "price.2") {
                remain__ = remain__ - floor(stockHold * table.price__[i]);
            }
            else {
                remain__ = remain__ - stockHold * table.price__[i];
            }
            buyNum__++;
            if (isRecordOn__) {
                record_buy_info(table, i, stockHold);
            }
        }
        else if (check_sell_cross(stockHold, table.MAtable__[i - 1][sell1_dec__], table.MAtable__[i - 1][sell2_dec__], table.MAtable__[i][sell1_dec__], table.MAtable__[i][sell2_dec__], i, endRow)) {
            if (_pricePath == "price.2") {
                remain__ = remain__ + floor((double)stockHold * table.price__[i]);
            }
            else {
                remain__ = remain__ + (double)stockHold * table.price__[i];
            }
            stockHold = 0;
            sellNum__++;
            if (isRecordOn__) {
                record_sell_info(table, i, stockHold);
            }
        }
    }
    if (buyNum__ != sellNum__) {
        cout << buyNum__ << "," << sellNum__ << endl;
        cout << "particle.buyNum__ != particle.sellNum__" << endl;
        exit(1);
    }
    RoR__ = (remain__ - TOTAL_CP_LV) / TOTAL_CP_LV * 100;
    if (isRecordOn__ && lastRecord) {
        record_last_info();
    }
}

void MA_GNQTS::Particle::print_trade_record(ofstream &out) {
    for_each(tradeRecord__.begin(), tradeRecord__.end(), [&out](auto &record) {
        out << record;
    });
}

void MA_GNQTS::Particle::print(ofstream &out, bool debug) {
    if (debug) {
        for_each(buy1_bi__.begin(), buy1_bi__.end(), [&out](auto i) { out << i << ","; });
        out << ",";
        for_each(buy2_bi__.begin(), buy2_bi__.end(), [&out](auto i) { out << i << ","; });
        out << ",";
        for_each(sell1_bi__.begin(), sell1_bi__.end(), [&out](auto i) { out << i << ","; });
        out << ",";
        for_each(sell2_bi__.begin(), sell2_bi__.end(), [&out](auto i) { out << i << ","; });
        out << ",";
        out << set_precision(RoR__) << "%," << buy1_dec__ << "," << buy2_dec__ << "," << sell1_dec__ << "," << sell2_dec__ << endl;
    }
    else {
        for_each(buy1_bi__.begin(), buy1_bi__.end(), [](auto i) { cout << i << ","; });
        cout << "|";
        for_each(buy2_bi__.begin(), buy2_bi__.end(), [](auto i) { cout << i << ","; });
        cout << "|";
        for_each(sell1_bi__.begin(), sell1_bi__.end(), [](auto i) { cout << i << ","; });
        cout << "|";
        for_each(sell2_bi__.begin(), sell2_bi__.end(), [](auto i) { cout << i << ","; });
        cout << "|";
        cout << set_precision(RoR__) << "%," << buy1_dec__ << "," << buy2_dec__ << "," << sell1_dec__ << "," << sell2_dec__ << endl;
    }
}

MA_GNQTS::Particle::Particle(int buy1, int buy2, int sell1, int sell2, bool on) : buy1_bi__(BUY1_BITS, 0), buy2_bi__(BUY2_BITS, 0), sell1_bi__(SELL1_BITS, 0), sell2_bi__(SELL1_BITS, 0), buy1_dec__(buy1), buy2_dec__(buy2), sell1_dec__(sell1), sell2_dec__(sell2), isRecordOn__(on) {
}

MA_GNQTS::BetaMatrix::BetaMatrix() : buy1__(BUY1_BITS, 0.5), buy2__(BUY2_BITS, 0.5), sell1__(SELL1_BITS, 0.5), sell2__(SELL2_BITS, 0.5) {
}

void MA_GNQTS::GNQTS() {
    for (int i = 0; i < BUY1_BITS; i++) {
        if (globalBest_.buy1_bi__[i] == 1 && localWorst_.buy1_bi__[i] == 0 && betaMtrix_.buy1__[i] < 0.5) {
            betaMtrix_.buy1__[i] = 1.0 - betaMtrix_.buy1__[i];
        }
        if (globalBest_.buy1_bi__[i] == 0 && localWorst_.buy1_bi__[i] == 1 && betaMtrix_.buy1__[i] > 0.5) {
            betaMtrix_.buy1__[i] = 1.0 - betaMtrix_.buy1__[i];
        }
        if (globalBest_.buy1_bi__[i] == 1) {
            betaMtrix_.buy1__[i] += delta_;
        }
        if (localWorst_.buy1_bi__[i] == 1) {
            betaMtrix_.buy1__[i] -= delta_;
        }
    }
    for (int i = 0; i < BUY2_BITS; i++) {
        if (globalBest_.buy2_bi__[i] == 1 && localWorst_.buy2_bi__[i] == 0 && betaMtrix_.buy2__[i] < 0.5) {
            betaMtrix_.buy2__[i] = 1.0 - betaMtrix_.buy2__[i];
        }
        if (globalBest_.buy2_bi__[i] == 0 && localWorst_.buy2_bi__[i] == 1 && betaMtrix_.buy2__[i] > 0.5) {
            betaMtrix_.buy2__[i] = 1.0 - betaMtrix_.buy2__[i];
        }
        if (globalBest_.buy2_bi__[i] == 1) {
            betaMtrix_.buy2__[i] += delta_;
        }
        if (localWorst_.buy2_bi__[i] == 1) {
            betaMtrix_.buy2__[i] -= delta_;
        }
    }
    for (int i = 0; i < SELL1_BITS; i++) {
        if (globalBest_.sell1_bi__[i] == 1 && localWorst_.sell1_bi__[i] == 0 && betaMtrix_.sell1__[i] < 0.5) {
            betaMtrix_.sell1__[i] = 1.0 - betaMtrix_.sell1__[i];
        }
        if (globalBest_.sell1_bi__[i] == 0 && localWorst_.sell1_bi__[i] == 1 && betaMtrix_.sell1__[i] > 0.5) {
            betaMtrix_.sell1__[i] = 1.0 - betaMtrix_.sell1__[i];
        }
        if (globalBest_.sell1_bi__[i] == 1) {
            betaMtrix_.sell1__[i] += delta_;
        }
        if (localWorst_.sell1_bi__[i] == 1) {
            betaMtrix_.sell1__[i] -= delta_;
        }
    }
    for (int i = 0; i < SELL2_BITS; i++) {
        if (globalBest_.sell2_bi__[i] == 1 && localWorst_.sell2_bi__[i] == 0 && betaMtrix_.sell2__[i] < 0.5) {
            betaMtrix_.sell2__[i] = 1.0 - betaMtrix_.sell2__[i];
        }
        if (globalBest_.sell2_bi__[i] == 0 && localWorst_.sell2_bi__[i] == 1 && betaMtrix_.sell2__[i] > 0.5) {
            betaMtrix_.sell2__[i] = 1.0 - betaMtrix_.sell2__[i];
        }
        if (globalBest_.sell2_bi__[i] == 1) {
            betaMtrix_.sell2__[i] += delta_;
        }
        if (localWorst_.sell2_bi__[i] == 1) {
            betaMtrix_.sell2__[i] -= delta_;
        }
    }
}

void MA_GNQTS::GQTS() {
    for (int i = 0; i < BUY1_BITS; i++) {
        if (globalBest_.buy1_bi__[i] == 1) {
            betaMtrix_.buy1__[i] += delta_;
        }
        if (localWorst_.buy1_bi__[i] == 1) {
            betaMtrix_.buy1__[i] -= delta_;
        }
    }
    for (int i = 0; i < BUY2_BITS; i++) {
        if (globalBest_.buy2_bi__[i] == 1) {
            betaMtrix_.buy2__[i] += delta_;
        }
        if (localWorst_.buy2_bi__[i] == 1) {
            betaMtrix_.buy2__[i] -= delta_;
        }
    }
    for (int i = 0; i < SELL1_BITS; i++) {
        if (globalBest_.sell1_bi__[i] == 1) {
            betaMtrix_.sell1__[i] += delta_;
        }
        if (localWorst_.sell1_bi__[i] == 1) {
            betaMtrix_.sell1__[i] -= delta_;
        }
    }
    for (int i = 0; i < SELL2_BITS; i++) {
        if (globalBest_.sell2_bi__[i] == 1) {
            betaMtrix_.sell2__[i] += delta_;
        }
        if (localWorst_.sell2_bi__[i] == 1) {
            betaMtrix_.sell2__[i] -= delta_;
        }
    }
}

void MA_GNQTS::QTS() {
    for (int i = 0; i < BUY1_BITS; i++) {
        if (localBest_.buy1_bi__[i] == 1) {
            betaMtrix_.buy1__[i] += delta_;
        }
        if (localWorst_.buy1_bi__[i] == 1) {
            betaMtrix_.buy1__[i] -= delta_;
        }
    }
    for (int i = 0; i < BUY2_BITS; i++) {
        if (localBest_.buy2_bi__[i] == 1) {
            betaMtrix_.buy2__[i] += delta_;
        }
        if (localWorst_.buy2_bi__[i] == 1) {
            betaMtrix_.buy2__[i] -= delta_;
        }
    }
    for (int i = 0; i < SELL1_BITS; i++) {
        if (localBest_.sell1_bi__[i] == 1) {
            betaMtrix_.sell1__[i] += delta_;
        }
        if (localWorst_.sell1_bi__[i] == 1) {
            betaMtrix_.sell1__[i] -= delta_;
        }
    }
    for (int i = 0; i < SELL2_BITS; i++) {
        if (localBest_.sell2_bi__[i] == 1) {
            betaMtrix_.sell2__[i] += delta_;
        }
        if (localWorst_.sell2_bi__[i] == 1) {
            betaMtrix_.sell2__[i] -= delta_;
        }
    }
}

void MA_GNQTS::compare_bits(vector<int> &bestVector, vector<int> &worstVector, int bits) {
    for (int i = 0; i < bits; i++) {
        if (bestVector[i] != worstVector[i]) {
            compareNew_++;
        }
    }
}

void MA_GNQTS::compare_and_multiply() {
    compare_bits(localBest_.buy1_bi__, localWorst_.buy1_bi__, BUY1_BITS);
    compare_bits(localBest_.buy2_bi__, localWorst_.buy2_bi__, BUY2_BITS);
    compare_bits(localBest_.sell1_bi__, localWorst_.sell1_bi__, SELL1_BITS);
    compare_bits(localBest_.sell2_bi__, localWorst_.sell2_bi__, SELL2_BITS);
    if (compareNew_ > compareOld_) {
        delta_ *= multiplyUp_;
    }
    else {
        delta_ *= multiplyDown_;
    }
    compareOld_ = compareNew_;
    compareNew_ = 0;
}

void MA_GNQTS::update_global() {
    for (auto i : particles_) {
        if (i.RoR__ > localBest_.RoR__) {
            localBest_ = i;
        }
        if (i.RoR__ < localWorst_.RoR__) {
            localWorst_ = i;
        }
    }
    if (localBest_.RoR__ > globalBest_.RoR__) {
        globalBest_ = localBest_;
    }
    switch (_algoUse) {
        case 0: {
            if (localBest_.RoR__ > 0) {
                QTS();
            }
            break;
        }
        case 1: {
            if (globalBest_.RoR__ > 0) {
                GQTS();
            }
            break;
        }
        case 2: {
            if (globalBest_.RoR__ > 0) {
                GNQTS();
            }
            break;
        }
        case 3: {
            if (globalBest_.RoR__ > 0) {
                GNQTS();
            }
            compare_and_multiply();
            break;
        }
        default: {
            cout << "wrong algo" << endl;
            exit(1);
        }
    }
}

void MA_GNQTS::BetaMatrix::initilaize() {
    fill(buy1__.begin(), buy1__.end(), 0.5);
    fill(buy2__.begin(), buy2__.end(), 0.5);
    fill(sell1__.begin(), sell1__.end(), 0.5);
    fill(sell2__.begin(), sell2__.end(), 0.5);
}

void MA_GNQTS::BetaMatrix::print(ofstream &out, bool debug) {
    if (debug) {
        out << "beta matrix" << endl;
        for_each(buy1__.begin(), buy1__.end(), [&out](auto i) { out << set_precision(i) << ","; });
        out << ",";
        for_each(buy2__.begin(), buy2__.end(), [&out](auto i) { out << set_precision(i) << ","; });
        out << ",";
        for_each(sell1__.begin(), sell1__.end(), [&out](auto i) { out << set_precision(i) << ","; });
        out << ",";
        for_each(sell2__.begin(), sell2__.end(), [&out](auto i) { out << set_precision(i) << ","; });
        out << endl;
    }
    else {
        cout << "beta matrix" << endl;
        for_each(buy1__.begin(), buy1__.end(), [](auto i) { cout << set_precision(i) << ","; });
        cout << "|";
        for_each(buy2__.begin(), buy2__.end(), [](auto i) { cout << set_precision(i) << ","; });
        cout << "|";
        for_each(sell1__.begin(), sell1__.end(), [](auto i) { cout << set_precision(i) << ","; });
        cout << "|";
        for_each(sell2__.begin(), sell2__.end(), [](auto i) { cout << set_precision(i) << ","; });
        cout << endl;
    }
}

void MA_GNQTS::find_new_row(string startDate, string endDate) {
    if (startDate != "") {
        for (int i = 0; i < table_.days__; i++) {
            if (startDate == table_.date__[i]) {
                actualStartRow_ = i;
                break;
            }
        }
        for (int i = actualStartRow_; i < table_.days__; i++) {
            if (endDate == table_.date__[i]) {
                actualEndRow_ = i;
                break;
            }
        }
        if (actualStartRow_ == -1) {
            cout << "input trainStartDate is not found" << endl;
            exit(1);
        }
        if (actualEndRow_ == -1) {
            cout << "input trainEndDate is not found" << endl;
            exit(1);
        }
    }
}

void MA_GNQTS::set_row_and_break_conditioin(string startDate, int &windowIndex, int &intervalIndex, CompanyInfo::TrainWindow &window) {
    if (startDate != "") {  //如果有設定特定的日期，就只跑一個視窗跟一個區間
        windowIndex = company_.windowNumber_;
        intervalIndex = (int)window.interval__.size();
    }
    else {  //如果沒有設定特定的日期把區間就把row存進去
        actualStartRow_ = window.interval__[intervalIndex];
        actualEndRow_ = window.interval__[intervalIndex + 1];
    }
    cout << table_.date__[actualStartRow_] << "~" << table_.date__[actualEndRow_] << endl;
}

void MA_GNQTS::print_debug_beta(bool debug, ofstream &out) {
    if (debug) {
        switch (_algoUse) {
            case 0: {
                out << "local best" << endl;
                localBest_.print(out, debug);
                out << "local worst" << endl;
                localWorst_.print(out, debug);
                break;
            }
            case 1:
            case 2: {
                out << "global best" << endl;
                globalBest_.print(out, debug);
                out << "local worst" << endl;
                localWorst_.print(out, debug);
                break;
            }
            case 3: {
                out << "global best" << endl;
                globalBest_.print(out, debug);
                out << "local best" << endl;
                localBest_.print(out, debug);
                out << "local worst" << endl;
                localWorst_.print(out, debug);
                out << delta_ << endl;
                break;
            }
        }
        betaMtrix_.print(out, debug);
    }
}

void MA_GNQTS::update_best() {
    if (best_.RoR__ < globalBest_.RoR__) {
        best_ = globalBest_;
    }
    if (best_.buy1_dec__ == globalBest_.buy1_dec__ && best_.buy2_dec__ == globalBest_.buy2_dec__ && best_.sell1_dec__ == globalBest_.sell1_dec__ && best_.sell2_dec__ == globalBest_.sell2_dec__) {
        best_.bestCnt__++;
    }
}

void MA_GNQTS::is_record_on(bool record) {
    if (record) {
        for (auto &i : particles_) {
            i.isRecordOn__ = record;
        }
    }
}

CompanyInfo::TrainWindow MA_GNQTS::set_wondow(string &startDate, string &targetWindow, int &windowIndex) {
    string accuallWindow = _slidingWindows[windowIndex];
    if (targetWindow != "all") {  //如果有設定特定的視窗，這邊要設定視窗
        accuallWindow = targetWindow;
        windowIndex = company_.windowNumber_;
    }
    CompanyInfo::TrainWindow window(accuallWindow, company_);
    if (startDate == "") {  //如果沒有設定特定的日期，就印所有視窗日期
        window.print_train();
    }
    if (company_.trainFilePath_ == "") {
        window.windowName__ = "";
    }
    return window;
}

ofstream MA_GNQTS::set_debug_file(bool debug) {
    ofstream out;
    if (debug) {
        string delta = set_precision(_delta);
        delta.erase(delta.find_last_not_of('0') + 1, std::string::npos);
        out.open("debug_" + company_.companyName_ + "_" + company_.MAType_ + "_" + _algo[_algoUse] + "_" + delta + "_" + table_.date__[actualStartRow_] + "_" + table_.date__[actualEndRow_] + ".csv");
    }
    return out;
}

void MA_GNQTS::print_debug_exp(bool debug, int expCnt, ofstream &out) {
    if (debug) {
        out << "exp:" << expCnt << ",==========,==========" << endl;
    }
}

void MA_GNQTS::print_debug_gen(bool debug, int generation, ofstream &out) {
    if (debug) {
        out << "gen:" << generation << ",=====" << endl;
    }
}

void MA_GNQTS::print_debug_particle(bool debug, int i, ofstream &out) {
    if (debug) {
        particles_[i].print(out, debug);
    }
}

void MA_GNQTS::store_exp_gen(int expCnt, int generation) {
    for_each(particles_.begin(), particles_.end(), [generation, expCnt](auto &i) {
        i.gen__ = generation;
        i.exp__ = expCnt;
    });
}

void MA_GNQTS::start_gen(bool debug, int expCnt, int generation, ofstream &out) {
    print_debug_gen(debug, generation, out);
    localBest_.initialize();
    localWorst_.initialize(TOTAL_CP_LV);
    for (int i = 0; i < PARTICAL_AMOUNT; i++) {
        particles_[i].initialize();
        particles_[i].measure(betaMtrix_);
        particles_[i].convert_bi_dec();
        particles_[i].trade(actualStartRow_, actualEndRow_, table_);
        print_debug_particle(debug, i, out);
    }
    store_exp_gen(expCnt, generation);
    update_global();
    print_debug_beta(debug, out);
}

void MA_GNQTS::initialize_KNQTS() {
    delta_ = _delta;
    compareNew_ = 0;
    compareOld_ = 0;
}

void MA_GNQTS::start_exp(bool debug, int expCnt, ofstream &out) {
    print_debug_exp(debug, expCnt, out);
    globalBest_.initialize();
    betaMtrix_.initilaize();
    initialize_KNQTS();
    for (int generation = 0; generation < _generationNumber; generation++) {
        start_gen(debug, expCnt, generation, out);
    }
    update_best();
}

string MA_GNQTS::Particle::set_output_filePath(int actualEndRow, int actualStartRow, CompanyInfo &company, string &outputPath, CompanyInfo::MATable &table) {
    if (outputPath != "") {
        if (_testDeltaLoop > 0) {
            string folderName = _setWindow + "_" + to_string(_delta);
            create_directories(folderName);
            outputPath = folderName;
        }
        outputPath += "/";
    }
    else {
        string delta = set_precision(_delta);
        delta.erase(delta.find_last_not_of('0') + 1, std::string::npos);
        outputPath = company.MAType_ + "_" + company.companyName_ + "_" + _algo[_algoUse] + "_" + delta + "_";
    }
    return outputPath + table.date__[actualStartRow] + "_" + table.date__[actualEndRow] + ".csv";
}

void MA_GNQTS::Particle::print_train_test_data(CompanyInfo &company, CompanyInfo::MATable &table, string outputPath, int actualStartRow, int actualEndRow) {
    string filePath = set_output_filePath(actualEndRow, actualStartRow, company, outputPath, table);
    isRecordOn__ = true;
    remain__ = TOTAL_CP_LV;
    trade(actualStartRow, actualEndRow, table);
    ofstream out;
    out.open(filePath);
    out << "algo," << _algo[_algoUse] << endl;
    out << "delta," << set_precision(_delta) << endl;
    out << "exp," << _expNumber << endl;
    out << "gen," << _generationNumber << endl;
    out << "p amount," << PARTICAL_AMOUNT << endl;
    out << endl;
    out << "initial capital," << set_precision(TOTAL_CP_LV) << endl;
    out << "final capital," << set_precision(remain__) << endl;
    out << "final return," << set_precision(remain__ - TOTAL_CP_LV) << endl;
    out << endl;
    out << "buy1," << buy1_dec__ << endl;
    out << "buy2," << buy2_dec__ << endl;
    out << "sell1," << sell1_dec__ << endl;
    out << "sell2," << sell2_dec__ << endl;
    out << "trade," << sellNum__ << endl;
    out << "return rate," << set_precision(RoR__) << "%" << endl;
    out << endl;
    out << "best exp," << exp__ << endl;
    out << "best gen," << gen__ << endl;
    out << "best cnt," << bestCnt__ << endl;
    out << endl;
    print_trade_record(out);
    out.close();
}

MA_GNQTS::MA_GNQTS(CompanyInfo &company, CompanyInfo::MATable &table, string targetWindow, string startDate, string endDate, bool debug, bool record) : particles_(PARTICAL_AMOUNT), table_(table), company_(company) {
    find_new_row(startDate, endDate);  //如果有設定特定的日期，這邊要重新找row
    is_record_on(record);
    for (int windowIndex{0}; windowIndex < company.windowNumber_; windowIndex++) {
        CompanyInfo::TrainWindow window = set_wondow(startDate, targetWindow, windowIndex);
        srand(343);
        for (int intervalIndex = 0; intervalIndex < window.interval__.size(); intervalIndex += 2) {
            set_row_and_break_conditioin(startDate, windowIndex, intervalIndex, window);
            best_.initialize();
            ofstream out = set_debug_file(debug);
            for (int expCnt = 0; expCnt < _expNumber; expCnt++) {
                start_exp(debug, expCnt, out);
            }
            out.close();
            best_.print_train_test_data(company_, table_, company_.trainFilePath_ + window.windowName__, actualStartRow_, actualEndRow_);
            cout << best_.RoR__ << "%" << endl;
        }
        cout << "==========" << endl;
    }
}

void CompanyInfo::MATable::create_MAtable(CompanyInfo &company) {
    days__ = company.totalDays_ - company.longestTrainRow_;
    date__ = new string[days__];
    price__ = new double[days__];
    for (int i = company.longestTrainRow_, j = 0; i < company.totalDays_; i++, j++) {
        date__[j] = company.date_[i];
        price__[j] = company.price_[i];
    }
    MAtable__ = new double *[days__];
    for (int i = 0; i < days__; i++) {
        MAtable__[i] = new double[257];
    }
    vector<path> MAFilePath;
    if (_pricePath == "price.2") {
        MAFilePath = get_path(company.MAType_ + ".2/" + company.companyName_);
    }
    else {
        MAFilePath = get_path(company.MAType_ + "/" + company.companyName_);
    }
    if (MAFilePath.size() == 0) {
        cout << "no MA file" << endl;
        exit(1);
    }
    for (int i = 0; i < MAFilePath.size(); i++) {
        vector<vector<string>> MAFile = read_data(MAFilePath[i]);
        if (int(MAFile.size()) - days__ < 0) {
            cout << company.companyName_ << " MA file not old enougth" << endl;
            exit(1);
        }
        for (int j = 0, k = int(MAFile.size()) - days__; k < MAFile.size(); j++, k++) {
            MAtable__[j][i + 1] = stod(MAFile[k][1]);
        }
    }
}

CompanyInfo::MATable::MATable(CompanyInfo &company) {
    create_MAtable(company);
}

CompanyInfo::MATable::~MATable() {
    delete[] date__;
    delete[] price__;
    for (int i = 0; i < days__; i++) {
        delete[] MAtable__[i];
    }
    delete[] MAtable__;
}

void CompanyInfo::TrainWindow::find_train_interval() {
    char delimiter;
    vector<string> trainType = find_train_and_test_len(windowNameEx__, delimiter);
    if (windowName__ == "A2A") {
        interval__.push_back(company__.testStartRow_);
        interval__.push_back(company__.testEndRow_);
    }
    else if (delimiter == 'M') {
        find_M_train(trainType, delimiter);
    }
    else if (delimiter == 'W') {
        find_W_train(trainType, delimiter);
    }
    else if (delimiter == 'D') {
        find_D_train(trainType, delimiter);
    }
}

void CompanyInfo::TrainWindow::find_first_train_start_row(int trainPeriodLength, char delimiter) {
    if (delimiter == 'M') {
        for (int i = company__.testStartRow_ - 1, monthCount = 0; i >= 0; i--) {
            if (company__.date_[i].substr(5, 2) != company__.date_[i - 1].substr(5, 2)) {
                monthCount++;
                if (monthCount == trainPeriodLength) {
                    firstTrainStartRow__ = i;
                    break;
                }
            }
        }
    }
    else if (delimiter == 'D') {
        firstTrainStartRow__ = company__.testStartRow_ - trainPeriodLength;
    }
    else if (delimiter == 'W') {
        for (int i = company__.testStartRow_ - 1, j = 0; j < trainPeriodLength; i--) {
            int smallWeekDay = cal_weekday(company__.date_[i - 1]);
            int bigWeekDay = cal_weekday(company__.date_[i]);
            if (is_week_changed(company__, bigWeekDay, smallWeekDay, i, i - 1)) {
                j++;
                if (j == trainPeriodLength) {
                    firstTrainStartRow__ = i;
                    break;
                }
            }
        }
    }
    if (firstTrainStartRow__ == -1) {
        cout << windowName__ << " can not find trainStartRow " << trainPeriodLength << endl;
        exit(1);
    }
}

void CompanyInfo::TrainWindow::find_M_train(vector<string> trainType, char delimiter) {
    vector<int> startRow, endRow;
    int trainLength = stoi(trainType[0]);
    int intervalNum = -1;
    int testLength = -1;
        //=======================================找出第一個startRow
    if (trainType.size() == 2) {
        find_first_train_start_row(trainLength, delimiter);
        intervalNum = ceil(_testYearLength * 12.0 / stod(trainType[1]));
        testLength = stoi(trainType[1]);
    }
    else if (trainType.size() == 1) {
        find_first_train_start_row(12, delimiter);
        intervalNum = ceil(_testYearLength * 12.0 / stod(trainType[0]));
        testLength = stoi(trainType[0]);
    }
        //=======================================找出所有startRow
    startRow.push_back(firstTrainStartRow__);
    for (int i = firstTrainStartRow__, intervalCount = 1, monthCount = 0; intervalCount < intervalNum; i++) {
        if (company__.date_[i].substr(5, 2) != company__.date_[i + 1].substr(5, 2)) {
            monthCount++;
            if (monthCount == testLength) {
                startRow.push_back(i + 1);
                intervalCount++;
                monthCount = 0;
            }
        }
    }
        //=======================================找出第一個endRow
    if (trainType.size() == 2) {
        firstTrainEndRow__ = company__.testStartRow_ - 1;
        endRow.push_back(firstTrainEndRow__);
    }
    else if (trainType.size() == 1) {
        for (int i = firstTrainStartRow__, monthCount = 0; i < company__.totalDays_; i++) {
            if (company__.date_[i].substr(5, 2) != company__.date_[i + 1].substr(5, 2)) {
                monthCount++;
                if (monthCount == trainLength) {
                    endRow.push_back(i);
                    firstTrainEndRow__ = i + 1;
                    break;
                }
            }
        }
    }
        //=======================================找出所有endRow
    for (int i = firstTrainEndRow__ + 1, intervalCount = 1, monthCount = 0; intervalCount < intervalNum; i++) {
        if (company__.date_[i].substr(5, 2) != company__.date_[i + 1].substr(5, 2)) {
            monthCount++;
            if (monthCount == testLength) {
                endRow.push_back(i);
                intervalCount++;
                monthCount = 0;
            }
        }
    }
    check_startRowSize_endRowSize(int(startRow.size()), int(endRow.size()));
    for (int i = 0; i < startRow.size(); i++) {
        interval__.push_back(startRow[i]);
        interval__.push_back(endRow[i]);
    }
}

void CompanyInfo::TrainWindow::find_W_train(vector<string> trainType, char delimiter) {
    vector<int> startRow, endRow;
    int trainLength{stoi(trainType[0])};
    int testLength{stoi(trainType[1])};
    find_first_train_start_row(trainLength, delimiter);
    int smallWeekDay = -1;
    int bigWeekDay = -1;
    startRow.push_back(firstTrainStartRow__);
    for (int i = firstTrainStartRow__, j = 0; i < company__.testEndRow_ - trainLength * 5; i++) {
        smallWeekDay = cal_weekday(company__.date_[i]);
        bigWeekDay = cal_weekday(company__.date_[i + 1]);
        if (is_week_changed(company__, bigWeekDay, smallWeekDay, i + 1, i)) {
            j++;
            if (j == testLength) {
                startRow.push_back(i + 1);
                j = 0;
            }
        }
    }
    firstTrainEndRow__ = company__.testEndRow_ - 1;
    endRow.push_back(firstTrainEndRow__);
    for (int i = company__.testStartRow_, j = 0; i < company__.testEndRow_; i++) {
        smallWeekDay = cal_weekday(company__.date_[i]);
        bigWeekDay = cal_weekday(company__.date_[i + 1]);
        if (is_week_changed(company__, bigWeekDay, smallWeekDay, i + 1, i)) {
            j++;
            if (j == testLength) {
                endRow.push_back(i);
                j = 0;
            }
        }
    }
    
    check_startRowSize_endRowSize(int(startRow.size()), int(endRow.size()));
    for (int i = 0; i < startRow.size(); i++) {
        interval__.push_back(startRow[i]);
        interval__.push_back(endRow[i]);
    }
}

int CompanyInfo::TrainWindow::cal_weekday(string date) {
    int y = stoi(date.substr(0, 4));
    int m = stoi(date.substr(5, 2)) - 1;
    int d = stoi(date.substr(8, 2));
    tm time_in = {0, 0, 0, d, m, y - 1900};
    time_t time_temp = mktime(&time_in);
    const tm *time_out = localtime(&time_temp);
    return time_out->tm_wday;
}

bool CompanyInfo::TrainWindow::is_week_changed(CompanyInfo &comapny, int bigWeekDay, int smallWeekDay, int big_i, int small_i) {
    return (bigWeekDay < smallWeekDay ||
            stoi(comapny.date_[big_i].substr(8, 2)) - stoi(comapny.date_[small_i].substr(8, 2)) >= 7 ||
            (stoi(comapny.date_[big_i].substr(8, 2)) < stoi(comapny.date_[small_i].substr(8, 2)) && stoi(comapny.date_[big_i].substr(8, 2)) + 30 - stoi(comapny.date_[small_i].substr(8, 2)) >= 7));
}

void CompanyInfo::TrainWindow::find_D_train(vector<string> trainType, char delimiter) {
    vector<int> startRow, endRow;
    int trainLength{stoi(trainType[0])};
    int testLength = -1;
        //=======================================找出訓練期開始Row
    find_first_train_start_row(trainLength, delimiter);
    testLength = stoi(trainType[1]);
        //=======================================找出所有訓練區間
    for (int i = firstTrainStartRow__; i <= company__.testEndRow_ - trainLength; i += testLength) {
        startRow.push_back(i);
    }
    firstTrainEndRow__ = company__.testStartRow_ - 1;
    for (int i = firstTrainEndRow__; i < company__.testEndRow_; i += testLength) {
        endRow.push_back(i);
    }
    check_startRowSize_endRowSize(int(startRow.size()), int(endRow.size()));
    for (int i = 0; i < startRow.size(); i++) {
        interval__.push_back(startRow[i]);
        interval__.push_back(endRow[i]);
    }
}

vector<string> CompanyInfo::TrainWindow::find_train_and_test_len(string window, char &delimiter) {
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

void CompanyInfo::TrainWindow::check_startRowSize_endRowSize(int startRowSize, int endRowSize) {
    if (startRowSize != endRowSize) {
        cout << "startRowSize != endRowSize" << endl;
        exit(1);
    }
}

void CompanyInfo::TrainWindow::print_train() {
    cout << "train " << windowName__ << "=" << windowNameEx__ << endl;
    for (auto it = interval__.begin(); it != interval__.end(); it++) {
        cout << company__.date_[*it + company__.longestTrainRow_] << "~" << company__.date_[*(++it) + company__.longestTrainRow_] << endl;
    }
    cout << "==========" << endl;
}

CompanyInfo::TrainWindow::TrainWindow(string window, CompanyInfo &company) : windowName__(window), company__(company), windowNameEx__(_slidingWindowsEX[distance(_slidingWindows.begin(), find(_slidingWindows.begin(), _slidingWindows.end(), windowName__))]) {
    find_train_interval();
    for (int &i : interval__) {
        i -= company__.longestTrainRow_;
    }
}

void CompanyInfo::TestWindow::find_test_interval() {
    char delimiter;
    vector<string> testType = TrainWindow::find_train_and_test_len(windowNameEx__, delimiter);
    if (delimiter == 'M') {
        if (testType.size() == 2) {
            find_M_test(stoi(testType[1]));
        }
        else {
            find_M_test(stoi(testType[0]));
        }
    }
    else if (delimiter == 'W') {
        find_W_test(stoi(testType[1]));
    }
    else if (delimiter == 'D') {
        find_D_test(stoi(testType[1]));
    }
}

void CompanyInfo::TestWindow::find_M_test(int testLength) {
    vector<int> startRow, endRow;
    for (int i = company__.testStartRow_, j = testLength - 1; i <= company__.testEndRow_; i++) {
        if (company__.date_[i - 1].substr(5, 2) != company__.date_[i].substr(5, 2)) {
            j++;
            if (j == testLength) {
                startRow.push_back(i);
                j = 0;
            }
        }
    }
    for (int i = company__.testStartRow_, j = 0; i <= company__.testEndRow_; i++) {
        if (company__.date_[i].substr(5, 2) != company__.date_[i + 1].substr(5, 2)) {
            j++;
            if (j == testLength || i == company__.testEndRow_) {
                endRow.push_back(i);
                j = 0;
            }
        }
    }
    TrainWindow::check_startRowSize_endRowSize((int)startRow.size(), (int)endRow.size());
    for (int i = 0; i < startRow.size(); i++) {
        interval__.push_back(startRow[i]);
        interval__.push_back(endRow[i]);
    }
}

void CompanyInfo::TestWindow::find_D_test(int testLength) {
    vector<int> startRow, endRow;
    for (int i = company__.testStartRow_; i <= company__.testEndRow_; i += testLength) {
        startRow.push_back(i);
    }
    for (int i = company__.testStartRow_ + testLength - 1; i < company__.testEndRow_; i += testLength) {
        endRow.push_back(i);
    }
    if (startRow.size() > endRow.size()) {
        endRow.push_back(company__.testEndRow_);
    }
    TrainWindow::check_startRowSize_endRowSize((int)startRow.size(), (int)endRow.size());
    for (int i = 0; i < startRow.size(); i++) {
        interval__.push_back(startRow[i]);
        interval__.push_back(endRow[i]);
    }
}

void CompanyInfo::TestWindow::find_W_test(int testLength) {
    vector<int> startRow, endRow;
    int smallWeekDay = -1;
    int bigWeekDay = -1;
    startRow.push_back(company__.testStartRow_);
    for (int i = company__.testStartRow_, j = 0; i < company__.testEndRow_; i++) {
        smallWeekDay = TrainWindow::cal_weekday(company__.date_[i]);
        bigWeekDay = TrainWindow::cal_weekday(company__.date_[i + 1]);
        if (TrainWindow::is_week_changed(company__, bigWeekDay, smallWeekDay, i + 1, i)) {
            j++;
            if (j == testLength) {
                startRow.push_back(i + 1);
                j = 0;
            }
        }
    }
    
    for (int i = company__.testStartRow_, j = 0; i <= company__.testEndRow_; i++) {
        smallWeekDay = TrainWindow::cal_weekday(company__.date_[i]);
        bigWeekDay = TrainWindow::cal_weekday(company__.date_[i + 1]);
        if (TrainWindow::is_week_changed(company__, bigWeekDay, smallWeekDay, i + 1, i)) {
            j++;
            if (j == testLength) {
                endRow.push_back(i);
                j = 0;
            }
        }
    }
    if (startRow.size() > endRow.size()) {
        endRow.push_back(company__.testEndRow_);
    }
    TrainWindow::check_startRowSize_endRowSize((int)startRow.size(), (int)endRow.size());
    for (int i = 0; i < startRow.size(); i++) {
        interval__.push_back(startRow[i]);
        interval__.push_back(endRow[i]);
    }
}

void CompanyInfo::TestWindow::print_test() {
    cout << windowName__ << "=" << windowNameEx__ << endl;
    for (auto it = interval__.begin(); it != interval__.end(); it++) {
        cout << company__.date_[*it + company__.longestTrainRow_] << "~" << company__.date_[*(++it) + company__.longestTrainRow_] << endl;
    }
    cout << "==========" << endl;
}

CompanyInfo::TestWindow::TestWindow(string window, CompanyInfo &company) : windowName__(window), company__(company), windowNameEx__(_slidingWindowsEX[distance(_slidingWindows.begin(), find(_slidingWindows.begin(), _slidingWindows.end(), windowName__))]) {
    find_test_interval();
    for (int &i : interval__) {
        i -= company__.longestTrainRow_;
    }
}

void CompanyInfo::create_folder() {
    create_directories(MAType_ + "/" + companyName_);
    for_each_n(_slidingWindows.begin(), _slidingWindows.size(), [&](auto i) {
        create_directories(trainFilePath_ + i);
        create_directories(testFilePath_ + i);
        create_directories(trainTraditionFilePath_ + i);
        create_directories(testTraditionFilePath_ + i);
    });
}

void CompanyInfo::store_date_price(path priceFilePath) {
    vector<vector<string>> priceFile = read_data(priceFilePath);
    totalDays_ = (int)priceFile.size() - 1;
    date_ = new string[totalDays_];
    price_ = new double[totalDays_];
    for (int i = 1, j = 0; i <= totalDays_; i++) {
        date_[i - 1] = priceFile[i][0];
            //        if (priceFile[i][_closeCol] == "null") {
        if (!is_double(priceFile[i][_closeCol])) {
            price_[i - 1] = price_[i - 2];
        }
        else {
            price_[i - 1] = stod(priceFile[i][_closeCol]);
        }
        if (j == 0 && date_[i - 1].substr(0, 4) == _testStartYear) {
            testStartRow_ = i - 1;
            j++;
        }
        else if (j == 1 && date_[i - 1].substr(0, 4) == _testEndYear) {
            testEndRow_ = i - 2;
            j++;
        }
    }
    if (_outputDecimal2 == 1) {
        create_directories("price.2");
        ofstream out;
        out.open("price.2/" + companyName_ + ".csv");
        out << endl;
        for (int i = 0; i < totalDays_; i++) {
            out << date_[i] << ",,,,";
            out << fixed << setprecision(2) << price_[i] << endl;
        }
        out.close();
        create_directories(MAOutputPath_);
        output_MA();
    }
}

void CompanyInfo::store_MA_to_vector() {
    cout << "calculating " << companyName_ << " " << MAType_ << endl;
    vector<double> tmp;
    MAtable_.push_back(tmp);
    switch (MAType_[0]) {
        case 'S':
            for (int MA = 1; MA < 257; MA++) {
                for (int dateRow = MA - 1; dateRow < totalDays_; dateRow++) {
                    double MARangePriceSum = 0;
                    for (int i = dateRow, j = MA; j > 0; i--, j--) {
                        MARangePriceSum += price_[i];
                    }
                    tmp.push_back(MARangePriceSum / MA);
                }
                MAtable_.push_back(tmp);
                tmp.clear();
            }
            break;
        case 'W':
            break;
        case 'E':
            break;
    }
    cout << "done calculating" << endl;
}

void CompanyInfo::output_MA() {
    store_MA_to_vector();
    cout << "saving " << companyName_ << " " << MAType_ << endl;
    switch (MAType_[0]) {
        case 'S':
            for (int MA = 1; MA < 257; MA++) {
                if (MA % 10 == 0) {
                    cout << ".";
                }
                ofstream out;
                if (MA < 10) {
                    out.open(MAOutputPath_ + "/" + companyName_ + "_" + MAType_ + "_00" + to_string(MA) + ".csv");
                        //                    out.open(companyName_ + "_" + MAType_ + "_00" + to_string(MA) + ".csv");
                }
                else if (MA >= 10 && MA < 100) {
                    out.open(MAOutputPath_ + "/" + companyName_ + "_" + MAType_ + "_0" + to_string(MA) + ".csv");
                        //                    out.open(companyName_ + "_" + MAType_ + "_0" + to_string(MA) + ".csv");
                }
                else if (MA >= 100) {
                    out.open(MAOutputPath_ + "/" + companyName_ + "_" + MAType_ + "_" + to_string(MA) + ".csv");
                        //                    out.open(companyName_ + "_" + MAType_ + "_" + to_string(MA) + ".csv");
                }
                if (_outputDecimal2 == 1) {
                    for (int i = 0, dateRow = MA - 1; i < MAtable_[MA].size(); i++, dateRow++) {
                        out << fixed << setprecision(2) << date_[dateRow] << "," << MAtable_[MA][i] << endl;
                    }
                }
                else {
                    for (int i = 0, dateRow = MA - 1; i < MAtable_[MA].size(); i++, dateRow++) {
                        out << date_[dateRow] << "," << set_precision(MAtable_[MA][i]) << endl;
                    }
                }
                out.close();
            }
            cout << endl;
            break;
        case 'W':
            break;
        case 'E':
            break;
    }
}

void CompanyInfo::train(string targetWindow, string startDate, string endDate, bool debug, bool record) {
    MATable table(*this);
        //==========如果有指定日期，將window設定A2A，這樣就不用找每個區間，並將開始結束日期設定
    if (targetWindow == "debug") {
        debug = true;
        if (startDate.length() == 10 && endDate.length() == 10) {
            targetWindow = "A2A";
        }
        else {
            targetWindow = startDate;
            startDate = "";
        }
        trainFilePath_ = "";
    }
    else if (targetWindow.length() == 10 && startDate.length() == 10) {
        if (endDate == "record") {
            record = true;
        }
        endDate = startDate;
        startDate = targetWindow;
        targetWindow = "A2A";
        trainFilePath_ = "";
    }
    else if (targetWindow == "record") {
        record = true;
        targetWindow = "all";
    }
    if (_testDeltaLoop == 0) {
        MA_GNQTS runGNQTS(*this, table, targetWindow, startDate, endDate, debug, record);
    }
    else {
        for (int i = 0; i < _testDeltaLoop; i++) {
            MA_GNQTS runGNQTS(*this, table, targetWindow, startDate, endDate, debug, record);
            _delta -= _testDeltaGap;
        }
    }
}

void CompanyInfo::find_train_start_row(int trainPeriodLength, char delimiter) {
    trainStartRow_ = -1;
    trainEndRow_ = -1;
    if (delimiter == 'M') {
        for (int i = testStartRow_ - 1, monthCount = 0; i >= 0; i--) {
            if (date_[i].substr(5, 2) != date_[i - 1].substr(5, 2)) {
                monthCount++;
                if (monthCount == trainPeriodLength) {
                    trainStartRow_ = i;
                    break;
                }
            }
        }
    }
    else if (delimiter == 'D') {
        trainStartRow_ = testStartRow_ - trainPeriodLength;
        trainEndRow_ = testStartRow_ - 1;
    }
    if (trainStartRow_ == -1) {
        cout << companyName_ << " can not find trainStartRow " << trainPeriodLength << endl;
        exit(1);
    }
}

void CompanyInfo::find_longest_train_month_row() {
    char delimiter;
    for (int i = 0; i < windowNumber_; i++) {
        string trainMonth = TrainWindow::find_train_and_test_len(_slidingWindowsEX[i], delimiter)[0];
        if (delimiter == 'M' && stoi(trainMonth) > longestTrainMonth_) {
            longestTrainMonth_ = stoi(trainMonth);
        }
    }
    if (longestTrainMonth_ == 0) {
        longestTrainMonth_ = 12;
    }
    for (int i = testStartRow_ - 1, monthCount = 0; i >= 0; i--) {
        if (date_[i].substr(5, 2) != date_[i - 1].substr(5, 2)) {
            monthCount++;
            if (monthCount == longestTrainMonth_) {
                longestTrainRow_ = i;
                longestTrainRow_ -= 20;
                break;
            }
        }
    }
    if (longestTrainRow_ == -1) {
        cout << "can't find longestTrainRow_" << endl;
        exit(1);
    }
}

void CompanyInfo::output_MATable() {
    MATable table(*this);
    ofstream out;
    out.open(companyName_ + "_" + MAType_ + "_Table.csv");
    out << MAType_ << ",";
    for (int i = 1; i < 257; i++) {
        out << i << ",";
    }
    out << endl;
    for (int i = 0; i < table.days__; i++) {
        out << table.date__[i] << ",";
        for (int j = 1; j < 257; j++) {
            out << set_precision(table.MAtable__[i][j]) << ",";
        }
        out << endl;
    }
    out.close();
}

void CompanyInfo::print_train(string targetWindow) {
    if (targetWindow == "all") {
        for_each(_slidingWindows.begin(), _slidingWindows.end(), [&](auto i) {
            TrainWindow window(i, *this);
            window.print_train();
        });
    }
    else {
        TrainWindow window(targetWindow, *this);
        window.print_train();
    }
}

void CompanyInfo::test(string targetWindow) {
    MATable table(*this);
    CalculateTest runTest(*this, table, targetWindow);
}

void CompanyInfo::print_test(string targetWindow) {
    if (targetWindow == "all") {
        for_each(_slidingWindows.begin(), _slidingWindows.end(), [&](auto i) {
            TestWindow window(i, *this);
            window.print_test();
        });
    }
    else {
        TestWindow window(targetWindow, *this);
        window.print_test();
    }
}

void CompanyInfo::instant_trade(string startDate, string endDate, int buy1, int buy2, int sell1, int sell2) {
    MATable table(*this);
    int startRow = -1;
    int endRow = -1;
    for (int i = 0; i < table.days__; i++) {
        if (startDate == table.date__[i]) {
            startRow = i;
            break;
        }
    }
    for (int i = startRow; i < table.days__; i++) {
        if (endDate == table.date__[i]) {
            endRow = i;
            break;
        }
    }
    if (startRow == -1) {
        cout << "instant trade startDate is not found" << endl;
        exit(1);
    }
    if (endRow == -1) {
        cout << "instant trade endDate is not found" << endl;
        exit(1);
    }
    MA_GNQTS::Particle p(buy1, buy2, sell1, sell2, true);
    p.trade(startRow, endRow, table, true);
    ofstream out;
    out.open(companyName_ + "_instantTrade_" + startDate + "_" + endDate + "_" + to_string(buy1) + "_" + to_string(buy2) + "_" + to_string(sell1) + "_" + to_string(sell2) + ".csv");
    out << "company,startDate,endDate,buy1,buy2,sell1,sell2" << endl;
    out << companyName_ << "," << startDate << "," << endDate << "," << buy1 << "," << buy2 << "," << sell1 << "," << sell2 << "\r" << endl;
    p.print_trade_record(out);
    out.close();
}

CompanyInfo::CompanyInfo(path filePath, string MAUse) : companyName_(filePath.stem().string()), MAType_(MAUse), MAOutputPath_(MAType_ + "/" + companyName_), trainFilePath_(_outputPath + "/" + companyName_ + "/train/"), testFilePath_(_outputPath + "/" + companyName_ + "/test/"), windowNumber_(int(_slidingWindows.size())), trainTraditionFilePath_(_outputPath + "/" + companyName_ + "/trainTradition/"), testTraditionFilePath_(_outputPath + "/" + companyName_ + "/testTradition/") {
    if (_outputDecimal2 == 1) {
        MAOutputPath_ = MAType_ + ".2/" + companyName_;
    }
    store_date_price(filePath);
    create_folder();
    find_longest_train_month_row();
    find_train_start_row(longestTrainMonth_, 'M');
}

CompanyInfo::~CompanyInfo() {
    delete[] date_;
    delete[] price_;
}

class IRRout {
public:
    class WindowIRR {  //裝滑動視窗跟報酬率
    public:
        string window_;
        double GNQTSIRR_;
        double traditionIRR_;
        int rank_;
    };
    class Rank {
    public:
        string companyName_;
        vector<int> GNQTSWindowRank_;
        vector<int> traditionWindowRank_;
    };
    double totalCPLV_;
    double testLength_;
    vector<path> companyPricePath_;
    vector<string> slidingWindows_;
    WindowIRR tmp_;
    vector<Rank> companyWindowRank_;
    string outputPath_;
    
    void compute_record_window_RoR(vector<path> &strategies, vector<path>::iterator &strategyPath, ofstream &RoROut, double *totalRate, int i);
    void record_window_IRR(string &companyName, ofstream &testRoROut, ofstream &traditionRoROut, int windowIndex, vector<IRRout::WindowIRR> &windowRankList);
    void record_BH_IRR(int companyIndex, string &setMA, vector<IRRout::WindowIRR> &windowRankList);
    void sort_by_tradition_IRR(vector<IRRout::WindowIRR> &windowRankList);
    void sort_by_window_Name(vector<IRRout::WindowIRR> &windowRankList);
    void sort_by_GNQTS_IRR(vector<IRRout::WindowIRR> &windowRankList);
    void rank_window(vector<IRRout::WindowIRR> &windowRankList);
    void rank_tradition_GNQTS_window(string &companyName, vector<IRRout::WindowIRR> &windowRankList);
    void record_company_window_IRR(ofstream &IRROut, int companyIndex, string &setMA);
    vector<string> removeA2ASort();
    void output_window_rank();
    
    IRRout(double testLength, vector<path> &companyPricePath, vector<string> slidingWindows, string setMA, double totalCPLV, string outputPath);
};

void IRRout::compute_record_window_RoR(vector<path> &strategies, vector<path>::iterator &strategyPath, ofstream &RoROut, double *totalRate, int i) {
    string strategyName = strategyPath->stem().string();
    vector<vector<string>> testFile = read_data(*strategyPath);
    double RoR = stod(testFile[15][1]);
    RoROut << strategyName << "," << testFile[10][1] << "," << testFile[11][1] << "," << testFile[12][1] << "," << testFile[13][1] << "," << testFile[15][1] << endl;
    if (strategyPath == strategies.begin()) {
        totalRate[i] = RoR / 100.0 + 1.0;
    }
    else {
        totalRate[i] = totalRate[i] * (RoR / 100.0 + 1.0);
    }
}

void IRRout::record_window_IRR(string &companyName, ofstream &testRoROut, ofstream &traditionRoROut, int windowIndex, vector<IRRout::WindowIRR> &windowRankList) {
    string windowName = slidingWindows_[windowIndex];
    if (windowName != "A2A") {
        cout << windowName << endl;
        testRoROut << ",================" << windowName << "================" << endl;
        traditionRoROut << ",================" << windowName << "================" << endl;
        double totalRate[] = {0, 0};
        vector<path> GNQTSstrategies = get_path(outputPath_ + "/" + companyName + "/test/" + windowName);
        for (auto strategyPath = GNQTSstrategies.begin(); strategyPath != GNQTSstrategies.end(); strategyPath++) {
            compute_record_window_RoR(GNQTSstrategies, strategyPath, testRoROut, totalRate, 0);
        }
        vector<path> traditionStrategies = get_path(outputPath_ + "/" + companyName + "/testTradition/" + windowName);
        for (auto strategyPath = traditionStrategies.begin(); strategyPath != traditionStrategies.end(); strategyPath++) {
            compute_record_window_RoR(traditionStrategies, strategyPath, traditionRoROut, totalRate, 1);
        }
        if (GNQTSstrategies.size() != traditionStrategies.size()) {
            cout << "GNQTSstrategies.size() != traditionStrategies.size()" << endl;
            exit(1);
        }
        tmp_.window_ = windowName;
        tmp_.GNQTSIRR_ = pow(totalRate[0], 1.0 / testLength_) - 1.0;  //計算年化報酬;
        tmp_.traditionIRR_ = pow(totalRate[1], 1.0 / testLength_) - 1.0;
        totalRate[0]--;
        totalRate[1]--;
        windowRankList.push_back(tmp_);
        testRoROut << ",,,,,," << windowName << "," << set_precision(totalRate[0]) << "," << set_precision(tmp_.GNQTSIRR_) << endl;
        traditionRoROut << ",,,,,," << windowName << "," << set_precision(totalRate[1]) << "," << set_precision(tmp_.traditionIRR_) << endl;
    }
}

void IRRout::record_BH_IRR(int companyIndex, string &setMA, vector<IRRout::WindowIRR> &windowRankList) {
    tmp_.window_ = "B&H";
    CompanyInfo company(companyPricePath_[companyIndex], setMA);
    BH bh(company.date_[company.testStartRow_], company.date_[company.testEndRow_], company, totalCPLV_);
    tmp_.GNQTSIRR_ = pow(bh.BHRoR + 1, 1.0 / testLength_) - 1;
    tmp_.traditionIRR_ = tmp_.GNQTSIRR_;
    windowRankList.push_back(tmp_);
}

void IRRout::sort_by_tradition_IRR(vector<IRRout::WindowIRR> &windowRankList) {
    sort(windowRankList.begin(), windowRankList.end(), [](const WindowIRR &a, const WindowIRR &b) {
        return a.traditionIRR_ > b.traditionIRR_;
    });
}

void IRRout::sort_by_window_Name(vector<IRRout::WindowIRR> &windowRankList) {
    sort(windowRankList.begin(), windowRankList.end(), [](const WindowIRR &a, const WindowIRR &b) {
        return a.window_ < b.window_;
    });
}

void IRRout::sort_by_GNQTS_IRR(vector<IRRout::WindowIRR> &windowRankList) {
    sort(windowRankList.begin(), windowRankList.end(), [](const WindowIRR &a, const WindowIRR &b) {
        return a.GNQTSIRR_ > b.GNQTSIRR_;
    });
}

void IRRout::rank_window(vector<IRRout::WindowIRR> &windowRankList) {
    for (int i = 0; i < windowRankList.size(); i++) {
        windowRankList[i].rank_ = i + 1;
    }
}

void IRRout::rank_tradition_GNQTS_window(string &companyName, vector<IRRout::WindowIRR> &windowRankList) {
    Rank tmpRank;
    tmpRank.companyName_ = companyName;
    sort_by_tradition_IRR(windowRankList);
    rank_window(windowRankList);
    sort_by_window_Name(windowRankList);
    for (int i = 0; i < windowRankList.size(); i++) {
        tmpRank.traditionWindowRank_.push_back(windowRankList[i].rank_);
    }
    sort_by_GNQTS_IRR(windowRankList);
    rank_window(windowRankList);
    sort_by_window_Name(windowRankList);
    for (int i = 0; i < windowRankList.size(); i++) {
        tmpRank.GNQTSWindowRank_.push_back(windowRankList[i].rank_);
    }
    companyWindowRank_.push_back(tmpRank);
    sort_by_GNQTS_IRR(windowRankList);
}

void IRRout::record_company_window_IRR(ofstream &IRROut, int companyIndex, string &setMA) {
    vector<WindowIRR> windowRankList;
    string companyName = companyPricePath_[companyIndex].stem().string();
    cout << "=====" << companyName << "=====" << endl;
    IRROut << "=====" << companyName << "=====,GNQTS,Tradition" << endl;
    ofstream GNQTSRoROut, traditionRoROut;
    GNQTSRoROut.open(outputPath_ + "/" + companyName + "/" + companyName + "_testRoR.csv");  //輸出一間公司所有視窗的每個區間的策略及報酬率
    traditionRoROut.open(outputPath_ + "/" + companyName + "/" + companyName + "_traditionTestRoR.csv");
    for (int windowIndex = 0; windowIndex < slidingWindows_.size(); windowIndex++) {
        record_window_IRR(companyName, GNQTSRoROut, traditionRoROut, windowIndex, windowRankList);
    }
    GNQTSRoROut.close();
    traditionRoROut.close();
    record_BH_IRR(companyIndex, setMA, windowRankList);
    rank_tradition_GNQTS_window(companyName, windowRankList);
    for (int i = 0; i < windowRankList.size(); i++) {
        IRROut << windowRankList[i].window_ << "," << set_precision(windowRankList[i].GNQTSIRR_ * 100.0) << "," << set_precision(windowRankList[i].traditionIRR_ * 100.0) << endl;
    }
}

vector<string> IRRout::removeA2ASort() {
    vector<string> windowSort = slidingWindows_;
    auto A2AIter = find(windowSort.begin(), windowSort.end(), "A2A");
    windowSort.erase(A2AIter);
    windowSort.push_back("B&H");
    sort(windowSort.begin(), windowSort.end());
    return windowSort;
}

void IRRout::output_window_rank() {
    vector<string> windowSort = removeA2ASort();
    ofstream rankOut;
    rankOut.open("windowRank.csv");
    rankOut << "GNQTS window rank" << endl;
    rankOut << ",";
    for (auto &windowName : windowSort) {
        rankOut << windowName << ",";
    }
    rankOut << endl;
    for (auto &i : companyWindowRank_) {
        rankOut << i.companyName_ << ",";
        for (auto &j : i.GNQTSWindowRank_) {
            rankOut << j << ",";
        }
        rankOut << endl;
    }
    rankOut << "tradition window rank" << endl;
    for (auto &i : companyWindowRank_) {
        rankOut << i.companyName_ << ",";
        for (auto &j : i.traditionWindowRank_) {
            rankOut << j << ",";
        }
        rankOut << endl;
    }
    rankOut.close();
}

IRRout::IRRout(double testLength, vector<path> &companyPricePath, vector<string> slidingWindows, string setMA, double totalCPLV, string outputPath) : testLength_(testLength), companyPricePath_(companyPricePath), slidingWindows_(slidingWindows), totalCPLV_(totalCPLV), outputPath_(outputPath) {
    ofstream IRROut;
    IRROut.open("test_IRR.csv");
    for (int companyIndex = 0; companyIndex < companyPricePath_.size(); companyIndex++) {
        record_company_window_IRR(IRROut, companyIndex, setMA);
    }
    output_window_rank();
    IRROut.close();
    companyPricePath.clear();
}

int main(int argc, const char *argv[]) {
    time_point begin = steady_clock::now();
    vector<path> companyPricePath = get_path(_pricePath);
    string setCompany = _setCompany;
    string setWindow = _setWindow;
    int setMode = _mode;
    string setMA = _MA[_MAUse];
    for (int companyIndex = 0; companyIndex < companyPricePath.size(); companyIndex++) {
        path targetPath = companyPricePath[companyIndex];
        if (setCompany != "all") {
            for (auto i : companyPricePath) {
                if (i.stem() == setCompany) {
                    targetPath = i;
                    break;
                }
            }
            companyPricePath.clear();
            companyPricePath.push_back(targetPath);
        }
        CompanyInfo company(targetPath, setMA);
        cout << company.companyName_ << endl;
        switch (setMode) {
            case 0: {
                company.train(setWindow);
                break;
            }
            case 1: {
                company.test(setWindow);
                break;
            }
            case 2: {
                Tradition tradition(company, setWindow);
                break;
            }
            case 3: {
                IRRout outputIRR(_testYearLength, companyPricePath, _slidingWindows, setMA, TOTAL_CP_LV, _outputPath);
                break;
            }
            case 10: {
                    //                company.output_MA();
                    //                company.train("debug", "2020-01-02", "2021-06-30");
                    //                company.train("2012-01-03", "2012-12-31");
                    //                company.instant_trade("2020-01-02", "2021-06-30", 43, 236, 20, 95);
                break;
            }
        }
    }
    time_point end = steady_clock::now();
    cout << "time: " << duration_cast<milliseconds>(end - begin).count() / 1000.0 << " s" << endl;
    return 0;
}
