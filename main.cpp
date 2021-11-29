#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;
using namespace std::filesystem;
using namespace std::chrono;

#define PARTICAL_AMOUNT 10
#define TOTAL_CP_LV 10000000.0
#define BUY1_BITS 8
#define BUY2_BITS 8
#define SELL1_BITS 8
#define SELL2_BITS 8

path _pricePath = "price";
int _closeCol = 4;
string _testStartYear = "2012";
string _testEndYear = "2021";
double _testYearLength = stod(_testEndYear) - stod(_testStartYear);
vector<string> _slidingWindows{"A2A", "YYY2YYY", "YY2YY", "YY2Y", "YH2Y", "Y2Y", "Y2H", "Y2Q", "Y2M", "H#", "H2H", "H2Q", "H2M", "Q#", "Q2Q", "Q2M", "M#", "M2M", "20D10", "2D2"};
vector<string> _slidingWindowsEX{"A2A", "36M36", "24M24", "24M12", "18M12", "12M12", "12M6", "12M3", "12M1", "6M", "6M6", "6M3", "6M1", "3M", "3M3", "3M1", "1M", "1M1", "20D10", "2D2"};

int _mode = 0;
int _techIndex = 0;
int _MAUse = 0;
int _algoUse = 0;
string _algo[] = {"QTS", "GQTS", "GNQTS"};

double _delta = 0.003;
int _expNumber = 50;
int _generationNumber = 10000;

string _outputPath = "result";

vector<vector<string>> read_data(path filePath) {
    ifstream infile(filePath);
    vector<vector<string>> data;
    if (!infile) {
        cout << filePath.string() + " not found" << endl;
        exit(1);
    }
        //    cout << "reading " + filePath.filename().string() << endl;
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
        oneRow.clear();
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

double round(double input, int point) {
    double output = (int)(input * pow(10, point) + 0.5);
    return output / (double)pow(10, point);
}

class CompanyInfo {
public:
    class MATable {
    public:
        int days__;
        string *date__;
        double *price__;
        double **allMA__;
        CompanyInfo &company__;
        
        void create_MATable();
        MATable(CompanyInfo &);
        ~MATable();
    };
    class TrainWindow {
    public:
        string windowName__;
        string windowNameEx__;
        vector<int> interval__;
        int firstTrainStartRow__{-1};
        int firstTrainEndRow__{-1};
        CompanyInfo &company__;
        
        void find_train_interval();
        void find_first_train_start_row(int, char);
        void find_M_train(vector<string>, char);
        void find_D_train(vector<string>, char);
        void find_W_train(vector<string>, char);
        static vector<string> find_train_type(string, char &);
        static void check_startRowSize_endRowSize(int, int);
        void print_train();
        TrainWindow(string, CompanyInfo &);
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
    int longestTrainMonth_{0};
    int longestTrainRow_{-1};
    int windowNumber_;
    vector<vector<double>> MAtable_;
    
    void store_date_price(path);
    string create_folder();
    void store_MA_to_vector();
    void output_MA();
    void train(string, string, string, bool, bool);
    void find_train_start_row(int, char);
    void print_train(string);
    void find_longest_train_month_row();
    void output_MATable();
    void instant_trade(string, string, int, int, int, int);
    CompanyInfo(path, string);
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
        void print(ofstream &, bool);
        BetaMatrix();
    };
    class Particle {
    public:
        vector<int> buy1_bi__;
        vector<int> buy2_bi__;
        vector<int> sell1_bi__;
        vector<int> sell2_bi__;
        int buy1_dec__{0};
        int buy2_dec__{0};
        int sell1_dec__{0};
        int sell2_dec__{0};
        int buyNum__{0};
        int sellNum__{0};
        vector<string> tradeRecord__;
        double remain__{TOTAL_CP_LV};
        double RoR__{0};
        bool isRecordOn__{false};
        int gen__;
        int exp__;
        int bestCnt__;
        
        void print(ofstream &, bool);
        void initialize(double);
        void measure(BetaMatrix &);
        void convert_bi_dec();
        void record_buy_info(CompanyInfo::MATable &, int, int);
        void record_sell_info(CompanyInfo::MATable &, int, int);
        void record_last_info();
        static bool check_buy_cross(int, double, double, double, double, int, int);
        static bool check_sell_cross(int, double, double, double, double, int, int);
        void round_price(CompanyInfo::MATable &);
        void trade(int, int, CompanyInfo::MATable &, bool);
        void print_trade_record(ofstream &, CompanyInfo &);
        Particle(int buy1 = 0, int buy2 = 0, int sell1 = 0, int sell2 = 0, bool on = false);
    };
    
    vector<Particle> particles_;
    BetaMatrix betaMtrix_;
    Particle localBest_;
    Particle localWorst_;
    Particle globalBest_;
    Particle best_;
    int actualStartRow_{-1};
    int actualEndRow_{-1};
    CompanyInfo::MATable &MAtable_;
    CompanyInfo &company_;
        //    void initialize_local_particles();
        //    void measure(Particle &);
        //    void convert_bi_to_dec(Particle &);
        //    void GNQTStrade(int, int, Particle &);
    void update_global();
        //    void print_all_particle();
        //    void print_betaMatrix();
    void find_new_row(string, string);
    void set_row_and_break_conditioin(string, int &, int &, CompanyInfo::TrainWindow &);
    void GNQTS();
    void GQTS();
    void QTS();
    void print_debug_beta(bool debug, ofstream &out);
    void update_best();
    void is_record_on(bool record);
    CompanyInfo::TrainWindow set_wondow(string &, string &, int &windowIndex);
    ofstream set_debug_file(bool debug);
    void print_debug_exp(bool, int, ofstream &);
    void print_debug_gen(bool, int, ofstream &);
    void print_debug_particle(bool, int, ofstream &);
    void store_exp_gen(int, int);
    void start_gen(bool, int, int, ofstream &);
    void start_exp(bool, int, ofstream &);
    void print_train_data(CompanyInfo &, CompanyInfo::MATable &);
    
    MA_GNQTS(CompanyInfo &, CompanyInfo::MATable &, string, string, string, bool, bool);
};

void MA_GNQTS::Particle::initialize(double RoR = 0) {
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
}

void MA_GNQTS::Particle::measure(BetaMatrix &beta) {
    double r;
    for_each(buy1_bi__.begin(), buy1_bi__.end(), [&, j = 0](auto &i) mutable {
        r = rand() / (double)RAND_MAX;
        if (r < beta.buy1__[j++]) {
            i = 1;
        }
        else {
            i = 0;
        }
    });
    for_each(buy2_bi__.begin(), buy2_bi__.end(), [&, j = 0](auto &i) mutable {
        r = rand() / (double)RAND_MAX;
        if (r < beta.buy2__[j++]) {
            i = 1;
        }
        else {
            i = 0;
        }
    });
    for_each(sell1_bi__.begin(), sell1_bi__.end(), [&, j = 0](auto &i) mutable {
        r = rand() / (double)RAND_MAX;
        if (r < beta.sell1__[j++]) {
            i = 1;
        }
        else {
            i = 0;
        }
    });
    for_each(sell2_bi__.begin(), sell2_bi__.end(), [&, j = 0](auto &i) mutable {
        r = rand() / (double)RAND_MAX;
        if (r < beta.sell2__[j++]) {
            i = 1;
        }
        else {
            i = 0;
        }
    });
        //    for (int i = 0; i < BUY1_BITS; i++) {
        //        r = rand();
        //        r = r / (double)RAND_MAX;
        //        if (r < beta.buy1__[i]) {
        //            buy1_bi__[i] = 1;
        //        }
        //        else {
        //            buy1_bi__[i] = 0;
        //        }
        //    }
        //    for (int i = 0; i < BUY2_BITS; i++) {
        //        r = rand();
        //        r = r / (double)RAND_MAX;
        //        if (r < beta.buy2__[i]) {
        //            buy2_bi__[i] = 1;
        //        }
        //        else {
        //            buy2_bi__[i] = 0;
        //        }
        //    }
        //    for (int i = 0; i < SELL1_BITS; i++) {
        //        r = rand();
        //        r = r / (double)RAND_MAX;
        //        if (r < beta.sell1__[i]) {
        //            sell1_bi__[i] = 1;
        //        }
        //        else {
        //            sell1_bi__[i] = 0;
        //        }
        //    }
        //    for (int i = 0; i < SELL2_BITS; i++) {
        //        r = rand();
        //        r = r / (double)RAND_MAX;
        //        if (r < beta.sell2__[i]) {
        //            sell2_bi__[i] = 1;
        //        }
        //        else {
        //            sell2_bi__[i] = 0;
        //        }
        //    }
}

void MA_GNQTS::Particle::convert_bi_dec() {
    for (int i = 0, j = BUY1_BITS - 1; i < BUY1_BITS; i++, j--) {
        buy1_dec__ += pow(2, j) * buy1_bi__[i];
    }
    for (int i = 0, j = BUY2_BITS - 1; i < BUY2_BITS; i++, j--) {
        buy2_dec__ += pow(2, j) * buy2_bi__[i];
    }
    for (int i = 0, j = SELL1_BITS - 1; i < SELL1_BITS; i++, j--) {
        sell1_dec__ += pow(2, j) * sell1_bi__[i];
    }
    for (int i = 0, j = SELL2_BITS - 1; i < SELL2_BITS; i++, j--) {
        sell2_dec__ += pow(2, j) * sell2_bi__[i];
    }
}

void MA_GNQTS::Particle::record_buy_info(CompanyInfo::MATable &MAtable_, int i, int stockHold) {
    tradeRecord__.push_back("buy," + MAtable_.date__[i] + "," + to_string(MAtable_.price__[i]) + "," + to_string(MAtable_.allMA__[i - 1][buy1_dec__]) + "," + to_string(MAtable_.allMA__[i - 1][buy2_dec__]) + "," + to_string(MAtable_.allMA__[i][buy1_dec__]) + "," + to_string(MAtable_.allMA__[i][buy2_dec__]) + "," + to_string(stockHold) + "," + to_string(remain__) + "," + to_string(remain__ + stockHold * MAtable_.price__[i]) + "\r");
}

void MA_GNQTS::Particle::record_sell_info(CompanyInfo::MATable &MAtable_, int i, int stockHold) {
    tradeRecord__.push_back("sell," + MAtable_.date__[i] + "," + to_string(MAtable_.price__[i]) + "," + to_string(MAtable_.allMA__[i - 1][sell1_dec__]) + "," + to_string(MAtable_.allMA__[i - 1][sell2_dec__]) + "," + to_string(MAtable_.allMA__[i][sell1_dec__]) + "," + to_string(MAtable_.allMA__[i][sell2_dec__]) + "," + to_string(stockHold) + "," + to_string(remain__) + "," + to_string(remain__ + stockHold * MAtable_.price__[i]) + "\r\r");
}

void MA_GNQTS::Particle::record_last_info() {
    tradeRecord__.push_back("buyNum," + to_string(buyNum__) + ",sellNum," + to_string(sellNum__) + "\rremain," + to_string(remain__) + "\rreturn rate," + to_string(RoR__) + "%\r");
}

bool MA_GNQTS::Particle::check_buy_cross(int stockHold, double MAbuy1PreDay, double MAbuy2PreDay, double MAbuy1Today, double MAbuy2Today, int i, int endRow) {
    return stockHold == 0 && MAbuy1PreDay <= MAbuy2PreDay && MAbuy1Today > MAbuy2Today && i != endRow;
}

bool MA_GNQTS::Particle::check_sell_cross(int stockHold, double MAsell1PreDay, double MAsell2PreDay, double MAsell1Today, double MAsell2Today, int i, int endRow) {
    return stockHold != 0 && ((MAsell1PreDay >= MAsell2PreDay && MAsell1Today < MAsell2Today) || i == endRow);
}

void MA_GNQTS::Particle::round_price(CompanyInfo::MATable &table) {
    for (int i = 0; i < table.days__; i++) {
        table.price__[i] = round(table.price__[i], 2);
    }
}

void MA_GNQTS::Particle::trade(int trainStartRow, int trainEndRow, CompanyInfo::MATable &table, bool lastRecord = false) {
    int stockHold{0};
    if (isRecordOn__) {
        tradeRecord__.push_back(",date,price,preday 1,preday 2,today 1,today 2,stockHold,remain,capital lv\r");
    }
    if (buyNum__ != 0 || sellNum__ != 0) {
        buyNum__ = 0;
        sellNum__ = 0;
    }
    for (int i = trainStartRow; i <= trainEndRow; i++) {
        if (check_buy_cross(stockHold, table.allMA__[i - 1][buy1_dec__], table.allMA__[i - 1][buy2_dec__], table.allMA__[i][buy1_dec__], table.allMA__[i][buy2_dec__], i, trainEndRow) && remain__ >= table.price__[i]) {
            stockHold = floor(remain__ / table.price__[i]);
            remain__ = floor(remain__ - stockHold * table.price__[i]);
            buyNum__++;
            if (isRecordOn__) {
                record_buy_info(table, i, stockHold);
            }
        }
        else if (check_sell_cross(stockHold, table.allMA__[i - 1][sell1_dec__], table.allMA__[i - 1][sell2_dec__], table.allMA__[i][sell1_dec__], table.allMA__[i][sell2_dec__], i, trainEndRow)) {
            remain__ = floor(remain__ + (double)stockHold * table.price__[i]);
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

void MA_GNQTS::Particle::print_trade_record(ofstream &out, CompanyInfo &company) {
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
        for_each(sell2_bi__.begin(), sell2_bi__.end(), [&](auto i) { out << i << ","; });
        out << ",";
        out << RoR__ << "%," << buy1_dec__ << "," << buy2_dec__ << "," << sell1_dec__ << "," << sell2_dec__ << endl;
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
        cout << RoR__ << "%," << buy1_dec__ << "," << buy2_dec__ << "," << sell1_dec__ << "," << sell2_dec__ << endl;
    }
        //    cout << buyNum__ << "," << sellNum__ << endl;
        //    for (auto i : tradeRecord__) {
        //        cout << i;
        //    }
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
            betaMtrix_.buy1__[i] += _delta;
        }
        if (localWorst_.buy1_bi__[i] == 1) {
            betaMtrix_.buy1__[i] -= _delta;
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
            betaMtrix_.buy2__[i] += _delta;
        }
        if (localWorst_.buy2_bi__[i] == 1) {
            betaMtrix_.buy2__[i] -= _delta;
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
            betaMtrix_.sell1__[i] += _delta;
        }
        if (localWorst_.sell1_bi__[i] == 1) {
            betaMtrix_.sell1__[i] -= _delta;
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
            betaMtrix_.sell2__[i] += _delta;
        }
        if (localWorst_.sell2_bi__[i] == 1) {
            betaMtrix_.sell2__[i] -= _delta;
        }
    }
}

void MA_GNQTS::GQTS() {
    for (int i = 0; i < BUY1_BITS; i++) {
        if (globalBest_.buy1_bi__[i] == 1) {
            betaMtrix_.buy1__[i] += _delta;
        }
        if (localWorst_.buy1_bi__[i] == 1) {
            betaMtrix_.buy1__[i] -= _delta;
        }
    }
    for (int i = 0; i < BUY2_BITS; i++) {
        if (globalBest_.buy2_bi__[i] == 1) {
            betaMtrix_.buy2__[i] += _delta;
        }
        if (localWorst_.buy2_bi__[i] == 1) {
            betaMtrix_.buy2__[i] -= _delta;
        }
    }
    for (int i = 0; i < SELL1_BITS; i++) {
        if (globalBest_.sell1_bi__[i] == 1) {
            betaMtrix_.sell1__[i] += _delta;
        }
        if (localWorst_.sell1_bi__[i] == 1) {
            betaMtrix_.sell1__[i] -= _delta;
        }
    }
    for (int i = 0; i < SELL2_BITS; i++) {
        if (globalBest_.sell2_bi__[i] == 1) {
            betaMtrix_.sell2__[i] += _delta;
        }
        if (localWorst_.sell2_bi__[i] == 1) {
            betaMtrix_.sell2__[i] -= _delta;
        }
    }
}

void MA_GNQTS::QTS() {
    for (int i = 0; i < BUY1_BITS; i++) {
        if (localBest_.buy1_bi__[i] == 1) {
            betaMtrix_.buy1__[i] += _delta;
        }
        if (localWorst_.buy1_bi__[i] == 1) {
            betaMtrix_.buy1__[i] -= _delta;
        }
    }
    for (int i = 0; i < BUY2_BITS; i++) {
        if (localBest_.buy2_bi__[i] == 1) {
            betaMtrix_.buy2__[i] += _delta;
        }
        if (localWorst_.buy2_bi__[i] == 1) {
            betaMtrix_.buy2__[i] -= _delta;
        }
    }
    for (int i = 0; i < SELL1_BITS; i++) {
        if (localBest_.sell1_bi__[i] == 1) {
            betaMtrix_.sell1__[i] += _delta;
        }
        if (localWorst_.sell1_bi__[i] == 1) {
            betaMtrix_.sell1__[i] -= _delta;
        }
    }
    for (int i = 0; i < SELL2_BITS; i++) {
        if (localBest_.sell2_bi__[i] == 1) {
            betaMtrix_.sell2__[i] += _delta;
        }
        if (localWorst_.sell2_bi__[i] == 1) {
            betaMtrix_.sell2__[i] -= _delta;
        }
    }
}

void MA_GNQTS::update_global() {
    localBest_ = particles_.front();
    localWorst_ = particles_.back();
    if (localBest_.RoR__ > globalBest_.RoR__) {
        globalBest_ = localBest_;
    }
    if (globalBest_.RoR__ != 0) {
        switch (_algoUse) {
            case 0: {
                QTS();
                break;
            }
            case 1: {
                GQTS();
                break;
            }
            case 2: {
                GNQTS();
                break;
            }
            default: {
                cout << "wrong algo" << endl;
                exit(1);
            }
        }
    }
}

    //void MA_GNQTS::print_all_particle() {
    //    for (int i = 0; i < particles_.size(); i++) {
    //        cout << "P:" << i << endl;
    //        for_each(particles_[i].buy1_bi__.begin(), particles_[i].buy1_bi__.end(), [](auto i) { cout << i << ","; });
    //        cout << "|";
    //        for_each(particles_[i].buy2_bi__.begin(), particles_[i].buy2_bi__.end(), [](auto i) { cout << i << ","; });
    //        cout << "|";
    //        for_each(particles_[i].sell1_bi__.begin(), particles_[i].sell1_bi__.end(), [](auto i) { cout << i << ","; });
    //        cout << "|";
    //        for_each(particles_[i].sell2_bi__.begin(), particles_[i].sell2_bi__.end(), [](auto i) { cout << i << ","; });
    //        cout << "|";
    //        cout << particles_[i].RoR__ << "%" << endl;
    //        cout << particles_[i].buy1_dec__ << "," << particles_[i].buy2_dec__ << "," << particles_[i].sell1_dec__ << "," << particles_[i].sell2_dec__ << endl;
    //    }
    //}

void MA_GNQTS::BetaMatrix::initilaize() {
    fill(buy1__.begin(), buy1__.end(), 0.5);
    fill(buy2__.begin(), buy2__.end(), 0.5);
    fill(sell1__.begin(), sell1__.end(), 0.5);
    fill(sell1__.begin(), sell2__.end(), 0.5);
}

void MA_GNQTS::BetaMatrix::print(ofstream &out, bool debug) {
    if (debug) {
        out << "beta matrix" << endl;
        for_each(buy1__.begin(), buy1__.end(), [&out](auto i) { out << i << ","; });
        out << ",";
        for_each(buy2__.begin(), buy2__.end(), [&out](auto i) { out << i << ","; });
        out << ",";
        for_each(sell1__.begin(), sell1__.end(), [&out](auto i) { out << i << ","; });
        out << ",";
        for_each(sell2__.begin(), sell2__.end(), [&out](auto i) { out << i << ","; });
        out << endl;
    }
    else {
        cout << "beta matrix" << endl;
        for_each(buy1__.begin(), buy1__.end(), [](auto i) { cout << i << ","; });
        cout << "|";
        for_each(buy2__.begin(), buy2__.end(), [](auto i) { cout << i << ","; });
        cout << "|";
        for_each(sell1__.begin(), sell1__.end(), [](auto i) { cout << i << ","; });
        cout << "|";
        for_each(sell2__.begin(), sell2__.end(), [](auto i) { cout << i << ","; });
        cout << endl;
    }
}

    //void MA_GNQTS::print_betaMatrix() {
    //    for (int i = 0; i < BUY1_BITS; i++) {
    //        cout << betaMtrix_.buy1__[i] << ",";
    //    }
    //    cout << "|";
    //    for (int i = 0; i < BUY2_BITS; i++) {
    //        cout << betaMtrix_.buy2__[i] << ",";
    //    }
    //    cout << "|";
    //    for (int i = 0; i < SELL1_BITS; i++) {
    //        cout << betaMtrix_.sell1__[i] << ",";
    //    }
    //    cout << "|";
    //    for (int i = 0; i < SELL2_BITS; i++) {
    //        cout << betaMtrix_.sell1__[i] << ",";
    //    }
    //    cout << endl;
    //}

void MA_GNQTS::find_new_row(string startDate, string endDate) {
    if (startDate != "") {
        for (int i = 0; i < MAtable_.days__; i++) {
            if (startDate == MAtable_.date__[i]) {
                actualStartRow_ = i;
                break;
            }
        }
        for (int i = actualStartRow_; i < MAtable_.days__; i++) {
            if (endDate == MAtable_.date__[i]) {
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
    cout << MAtable_.date__[actualStartRow_] << "~" << MAtable_.date__[actualEndRow_] << endl;
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
    return window;
}

ofstream MA_GNQTS::set_debug_file(bool debug) {
    ofstream out;
    if (debug) {
        out.open("debug_" + MAtable_.date__[actualStartRow_] + "_" + MAtable_.date__[actualEndRow_] + ".csv");
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
    stable_sort(particles_.begin(), particles_.end(), [](const Particle &p1, const Particle &p2) { return p1.RoR__ > p2.RoR__; });
}

void MA_GNQTS::start_gen(bool debug, int expCnt, int generation, ofstream &out) {
    print_debug_gen(debug, generation, out);
    localBest_.initialize();
    localWorst_.initialize(TOTAL_CP_LV);
    for (int i = 0; i < PARTICAL_AMOUNT; i++) {
        particles_[i].initialize();
        particles_[i].measure(betaMtrix_);
        particles_[i].convert_bi_dec();
        particles_[i].trade(actualStartRow_, actualEndRow_, MAtable_);
        print_debug_particle(debug, i, out);
    }
    store_exp_gen(expCnt, generation);
    update_global();
    print_debug_beta(debug, out);
}

void MA_GNQTS::start_exp(bool debug, int expCnt, ofstream &out) {
    print_debug_exp(debug, expCnt, out);
    globalBest_.initialize();
    betaMtrix_.initilaize();
    for (int generation = 0; generation < _generationNumber; generation++) {
        start_gen(debug, expCnt, generation, out);
    }
    update_best();
}

void MA_GNQTS::print_train_data(CompanyInfo &company, CompanyInfo::MATable &table) {
    ofstream trainedData;
    trainedData.open(table.date__[actualStartRow_] + "_" + table.date__[actualEndRow_] + ".csv");
    trainedData << "algo," + _algo[_algoUse] << endl;
    trainedData << "delta," << _delta << endl;
    trainedData << "exp," << _expNumber << endl;
    trainedData << "gen," << _generationNumber << endl;
    trainedData << "p amount," << PARTICAL_AMOUNT << endl;
    trainedData << endl;
    trainedData << "initial capital," << TOTAL_CP_LV << endl;
    trainedData << "final capital," << best_.remain__ << endl;
    trainedData << "final return," << best_.remain__ - TOTAL_CP_LV << endl;
    trainedData << endl;
    trainedData << "buy1," << best_.buy1_dec__ << endl;
    trainedData << "buy2," << best_.buy2_dec__ << endl;
    trainedData << "sell1," << best_.sell1_dec__ << endl;
    trainedData << "sell2," << best_.sell2_dec__ << endl;
    trainedData << "trade," << best_.sellNum__ << endl;
    trainedData << "return rate," << best_.RoR__ << "%" << endl;
    trainedData << endl;
    trainedData << "best exp," << best_.exp__ << endl;
    trainedData << "best gen," << best_.gen__ << endl;
    trainedData << "best cnt," << best_.bestCnt__ << endl;
    trainedData << endl;
    best_.isRecordOn__ = true;
    best_.remain__ = TOTAL_CP_LV;
    best_.trade(actualStartRow_, actualEndRow_, table);
    best_.print_trade_record(trainedData, company);
    trainedData.close();
    cout << best_.RoR__ << "%" << endl;
}

MA_GNQTS::MA_GNQTS(CompanyInfo &company, CompanyInfo::MATable &table, string targetWindow, string startDate, string endDate, bool debug, bool record) : particles_(PARTICAL_AMOUNT), MAtable_(table), company_(company) {
    for (int i = 0; i < table.days__; i++) {
        table.price__[0] = round(table.price__[i], 2);
        for (int j = 1; j < 257; j++) {
            table.allMA__[i][j] = round(table.allMA__[i][j], 2);
        }
    }
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
            print_train_data(company, table);
        }
        cout << "==========" << endl;
    }
}

void CompanyInfo::MATable::create_MATable() {
    days__ = company__.totalDays_ - company__.longestTrainRow_;
    date__ = new string[days__];
    price__ = new double[days__];
    for (int i = company__.longestTrainRow_, j = 0; i < company__.totalDays_; i++, j++) {
        date__[j] = company__.date_[i];
        price__[j] = company__.price_[i];
    }
    allMA__ = new double *[days__];
    for (int i = 0; i < days__; i++) {
        allMA__[i] = new double[257];
    }
    vector<path> MAFilePath = get_path(company__.MAType_ + "/" + company__.companyName_);
    for (int i = 0; i < MAFilePath.size(); i++) {
        vector<vector<string>> MAFile = read_data(MAFilePath[i]);
        if (int(MAFile.size()) - days__ < 0) {
            cout << company__.companyName_ + " MA file not old enougth" << endl;
            exit(1);
        }
        for (int j = 0, k = int(MAFile.size()) - days__; k < MAFile.size(); j++, k++) {
            allMA__[j][i + 1] = stod(MAFile[k][1]);
        }
    }
}

CompanyInfo::MATable::MATable(CompanyInfo &company) : company__(company) {
    create_MATable();
}

CompanyInfo::MATable::~MATable() {
    delete[] date__;
    delete[] price__;
    for (int i = 0; i < days__; i++) {
        delete[] allMA__[i];
    }
    delete[] allMA__;
}

void CompanyInfo::TrainWindow::find_train_interval() {
    char delimiter;
    vector<string> trainType = find_train_type(windowNameEx__, delimiter);
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
        firstTrainEndRow__ = company__.testStartRow_ - 1;
    }
    if (firstTrainStartRow__ == -1) {
        cout << windowName__ + " can not find trainStartRow " << trainPeriodLength << endl;
        exit(1);
    }
}

void CompanyInfo::TrainWindow::find_M_train(vector<string> trainType, char delimiter) {
    vector<int> startRow;
    vector<int> endRow;
    int trainPeriodLength = stoi(trainType[0]);
    int intervalNum{-1};
    int testPeriodLength{-1};
        //=======================================找出第一個startRow
    if (trainType.size() == 2) {
        find_first_train_start_row(trainPeriodLength, delimiter);
        intervalNum = ceil(_testYearLength * 12.0 / stod(trainType[1]));
        testPeriodLength = stoi(trainType[1]);
    }
    else if (trainType.size() == 1) {
        find_first_train_start_row(12, delimiter);
        intervalNum = ceil(_testYearLength * 12.0 / stod(trainType[0]));
        testPeriodLength = stoi(trainType[0]);
    }
        //=======================================找出所有startRow
    startRow.push_back(firstTrainStartRow__);
    for (int i = firstTrainStartRow__, intervalCount = 1, monthCount = 0; intervalCount < intervalNum; i++) {
        if (company__.date_[i].substr(5, 2) != company__.date_[i + 1].substr(5, 2)) {
            monthCount++;
            if (monthCount == testPeriodLength) {
                startRow.push_back(i + 1);
                intervalCount++;
                monthCount = 0;
            }
        }
    }
        //=======================================找出第一個endRow
    if (trainType.size() == 2) {
        endRow.push_back(company__.testStartRow_ - 1);
        firstTrainEndRow__ = company__.testStartRow_;
    }
    else if (trainType.size() == 1) {
        for (int i = firstTrainStartRow__, monthCount = 0; i < company__.totalDays_; i++) {
            if (company__.date_[i].substr(5, 2) != company__.date_[i + 1].substr(5, 2)) {
                monthCount++;
                if (monthCount == trainPeriodLength) {
                    endRow.push_back(i);
                    firstTrainEndRow__ = i + 1;
                    break;
                }
            }
        }
    }
        //=======================================找出所有endRow
    for (int i = firstTrainEndRow__, intervalCount = 1, monthCount = 0; intervalCount < intervalNum; i++) {
        if (company__.date_[i].substr(5, 2) != company__.date_[i + 1].substr(5, 2)) {
            monthCount++;
            if (monthCount == testPeriodLength) {
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
    vector<int> startRow;
    vector<int> endRow;
    int trainPeriodLength{stoi(trainType[0])};
    int intervalNum{-1};
    int testPeriodLength{-1};
        //=======================================找出訓練期開始Row
    find_first_train_start_row(trainPeriodLength, delimiter);
    intervalNum = _testYearLength * (12 / stoi(trainType[1]));
    testPeriodLength = stoi(trainType[1]);
        //=======================================找出所有訓練區間
    
        //=======================================
    check_startRowSize_endRowSize(int(startRow.size()), int(endRow.size()));
}

void CompanyInfo::TrainWindow::find_D_train(vector<string> trainType, char delimiter) {
    vector<int> startRow;
    vector<int> endRow;
    int trainPeriodLength{stoi(trainType[0])};
    int testPeriodLength{-1};
        //=======================================找出訓練期開始Row
    find_first_train_start_row(trainPeriodLength, delimiter);
    testPeriodLength = stoi(trainType[1]);
        //=======================================找出所有訓練區間
    for (int i = firstTrainStartRow__; i <= company__.testEndRow_ - trainPeriodLength; i += testPeriodLength) {
        startRow.push_back(i);
    }
    for (int i = firstTrainEndRow__; i < company__.testEndRow_; i += testPeriodLength) {
        endRow.push_back(i);
    }
    check_startRowSize_endRowSize(int(startRow.size()), int(endRow.size()));
    for (int i = 0; i < startRow.size(); i++) {
        interval__.push_back(startRow[i]);
        interval__.push_back(endRow[i]);
    }
}

vector<string> CompanyInfo::TrainWindow::find_train_type(string window, char &delimiter) {
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
    cout << windowName__ + "=" + windowNameEx__ << endl;
    for (auto it = interval__.begin(); it != interval__.end(); it++) {
        cout << company__.date_[*it + company__.longestTrainRow_] + "~" + company__.date_[*(++it) + company__.longestTrainRow_] << endl;
    }
    cout << "==========" << endl;
}

CompanyInfo::TrainWindow::TrainWindow(string window, CompanyInfo &company) : windowName__(window), company__(company), windowNameEx__(_slidingWindowsEX[distance(_slidingWindows.begin(), find(_slidingWindows.begin(), _slidingWindows.end(), windowName__))]) {
    find_train_interval();
    for (int &i : interval__) {
        i -= company__.longestTrainRow_;
    }
}

string CompanyInfo::create_folder() {
    create_directories(MAType_ + "/" + companyName_);
    return MAType_ + "/" + companyName_;
}

void CompanyInfo::store_date_price(path priceFilePath) {
    vector<vector<string>> priceFile = read_data(priceFilePath);
    totalDays_ = (int)priceFile.size() - 1;
    date_ = new string[totalDays_];
    price_ = new double[totalDays_];
    for (int i = 1, j = 0; i <= totalDays_; i++) {
        date_[i - 1] = priceFile[i][0];
        if (priceFile[i][_closeCol] == "null") {
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
}

void CompanyInfo::store_MA_to_vector() {
    cout << "calculating " << companyName_ + " " + MAType_ << endl;
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
    cout << "saving " << companyName_ + " " + MAType_ << endl;
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
                    //                for (int dateRow = MA - 1; dateRow < totalDays_; dateRow++) {
                    //                    double MARangePriceSum = 0;
                    //                    for (int i = dateRow, j = MA; j > 0; i--, j--) {
                    //                        MARangePriceSum += price_[i];
                    //                    }
                    //                    out << fixed << setprecision(2) << date_[dateRow] + "," << MARangePriceSum / MA << endl;
                    //                }
                for (int i = 0, dateRow = MA - 1; i < MAtable_[MA].size(); i++, dateRow++) {
                    out << date_[dateRow] + "," << MAtable_[MA][i] << endl;
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

void CompanyInfo::train(string targetWindow = "all", string startDate = "", string endDate = "", bool debug = false, bool record = false) {
    MATable table(*this);
        //==========如果有指定日期，將window設定A2A，這樣就不用找每個區間，並將開始結束日期設定
        //    if (startDate.length() != endDate.length() && startDate != "debug") {
        //        endDate = startDate;
        //        startDate = targetWindow;
        //        targetWindow = "A2A";
        //    }
        //    else if (targetWindow == "debug") {
        //        debug = true;
        //        record = false;
        //        targetWindow = "A2A";
        //    }
        //    else if (targetWindow != "all" && startDate == "debug") {
        //        startDate = "";
        //        debug = true;
        //    }
    
    if (targetWindow == "debug") {
        debug = true;
        if (startDate.length() == 10 && endDate.length() == 10) {
            targetWindow = "A2A";
        }
        else {
            targetWindow = startDate;
            startDate = "";
        }
    }
    else if (targetWindow.length() == 10 && startDate.length() == 10) {
        if (endDate == "record") {
            record = true;
        }
        endDate = startDate;
        startDate = targetWindow;
        targetWindow = "A2A";
    }
    else if (targetWindow == "record") {
        record = true;
        targetWindow = "all";
    }
    MA_GNQTS runGNQTS(*this, table, targetWindow, startDate, endDate, debug, record);
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
        cout << companyName_ + " can not find trainStartRow " << trainPeriodLength << endl;
        exit(1);
    }
}

void CompanyInfo::find_longest_train_month_row() {
    char delimiter;
    for (int i = 0; i < windowNumber_; i++) {
        string trainMonth = TrainWindow::find_train_type(_slidingWindowsEX[i], delimiter)[0];
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
    out << MAType_ + ",";
    for (int i = 1; i < 257; i++) {
        out << i << ",";
    }
    out << endl;
    for (int i = 0; i < table.days__; i++) {
        out << table.date__[i] + ",";
        for (int j = 1; j < 257; j++) {
            out << table.allMA__[i][j] << ",";
        }
        out << endl;
    }
    out.close();
}

void CompanyInfo::print_train(string targetWindow = "all") {
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

void CompanyInfo::instant_trade(string startDate, string endDate, int buy1, int buy2, int sell1, int sell2) {
    MATable table(*this);
    int startRow{-1};
    int endRow{-1};
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
    p.round_price(table);
    p.trade(startRow, endRow, table, true);
    ofstream out;
    out.open(companyName_ + "_instantTrade_" + startDate + "_" + endDate + "_" + to_string(p.buy1_dec__) + "_" + to_string(p.buy2_dec__) + "_" + to_string(p.sell1_dec__) + "_" + to_string(p.sell2_dec__) + ".csv");
    p.print_trade_record(out, *this);
    out.close();
}

CompanyInfo::CompanyInfo(path filePath, string MAUse) {
    companyName_ = filePath.stem().string();
    store_date_price(filePath);
    MAType_ = MAUse;
    MAOutputPath_ = create_folder();
    windowNumber_ = int(_slidingWindows.size());
    find_longest_train_month_row();
    find_train_start_row(longestTrainMonth_, 'M');
}

CompanyInfo::~CompanyInfo() {
    delete[] date_;
    delete[] price_;
}

int main(int argc, const char *argv[]) {
    string MAUse[] = {"SMA", "WMA", "EMA"};
    vector<path> companyPricePath = get_path(_pricePath);
    string setCompany = "2603.TW";
    for (int companyIndex = 0; companyIndex < companyPricePath.size(); companyIndex++) {
        path targetPath = companyPricePath[companyIndex];
        if (setCompany != "all") {
            for (auto i : companyPricePath) {
                if (i.stem() == setCompany) {
                    targetPath = i;
                    break;
                }
            }
        }
        CompanyInfo company(targetPath, MAUse[_MAUse]);
        cout << company.companyName_ << endl;
            //        company.output_MA();
            //        company.store_MA_to_vector();
            //        company.cal_MA_output();
            //        company.outputMATable();
            //        company.train("M2M");
            //        company.train("2020-01-02", "2021-06-30");
        company.train("2012-01-04", "2012-12-28");
            //        company.print_train();
            //        company.instant_trade("2020-01-02", "2021-06-30", 5,20,5,20);
        if (setCompany != "all") {
            break;
        }
    }
    return 0;
}

/*calculate time
 time_point begin = std::chrono::steady_clock::now();
 time_point end = std::chrono::steady_clock::now();
 cout << "time: " << duration_cast<seconds>(end - begin).count() << "[s]" << endl;
 cout << "time: " << duration_cast<microseconds>(end - begin).count() << "[µs]" << endl;
 cout << "time: " << duration_cast<nanoseconds>(end - begin).count() << "[µs]" << endl;
 */

/*測試particle交易用
 MA_GNQTS::Particle p;
 p.buy1_dec__ = 5;
 p.buy2_dec__ = 20;
 p.sell1_dec__ = 5;
 p.sell2_dec__ = 20;
 CompanyInfo::MATable table(company);
 int x{-1};
 int y{-1};
 for (int i = 0; i < table.trainDays__; i++) {
 if (table.tableDate__[i] == "2012-01-02") {
 x = i;
 break;
 }
 }
 for (int i = 0; i < table.trainDays__; i++) {
 if (table.tableDate__[i] == "2020-12-31") {
 y = i;
 break;
 }
 }
 MA_GNQTS::Trade(x, y, table, p, company);
 */

/*for instatnt trade
 MA_GNQTS::Trade trade1("2020-01-02", "2021-06-30", 5, 20, 5, 20, company);
 MA_GNQTS::Trade trade2("2020-01-02", "2021-06-30", 5, 60, 5, 60, company);
 MA_GNQTS::Trade trade3("2020-01-02", "2021-06-30", 6, 18, 6, 18, company);
 MA_GNQTS::Trade trade4("2020-01-02", "2021-06-30", 6, 50, 6, 50, company);
 MA_GNQTS::Trade trade5("2020-01-02", "2021-06-30", 18, 50, 18, 50, company);
 MA_GNQTS::Trade trade6("2020-01-02", "2021-06-30", 20, 60, 20, 60, company);
 */

/*檢查MAtable大小
 CompanyInfo::MATable table(company);
 cout << table.tableDate__[0] << endl;
 cout << table.tableDate__[table.trainDays__ - 1] << endl;
 */

/*
 class Trade {
 public:
 vector<double> MAbuy1__;
 vector<double> MAbuy2__;
 vector<double> MAsell1__;
 vector<double> MAsell2__;
 string startDate__;
 string endDate__;
 int buy1__;
 int buy2__;
 int sell1__;
 int sell2__;
 int startRow__{-1};
 int endRow__{-1};
 vector<string> tradeRecord__;
 CompanyInfo &company__;
 string *date__;
 double *price__;
 int buyNum__{0};
 int sellNum__{0};
 double RoR__{0};
 
 void cal_instant_trade_MA();
 void tradeDetail();
 void outputTradeRecord();
 static bool check_buy_cross(int, double, double, double, double, int, int);
 static bool check_sell_cross(int, double, double, double, double, int, int);
 void round_price();
 Trade(string, string, int, int, int, int, CompanyInfo &);
 Trade(int, int, CompanyInfo::MATable &, Particle &, CompanyInfo &);
 };
 */

/*
 bool MA_GNQTS::Trade::check_buy_cross(int stockHold, double MAbuy1PreDay, double MAbuy2PreDay, double MAbuy1Today, double MAbuy2Today, int i, int endRow) {
 return stockHold == 0 && MAbuy1PreDay <= MAbuy2PreDay && MAbuy1Today > MAbuy2Today && i != endRow;
 }
 
 bool MA_GNQTS::Trade::check_sell_cross(int stockHold, double MAsell1PreDay, double MAsell2PreDay, double MAsell1Today, double MAsell2Today, int i, int endRow) {
 return stockHold != 0 && ((MAsell1PreDay >= MAsell2PreDay && MAsell1Today < MAsell2Today) || i == endRow);
 }
 
 void MA_GNQTS::Trade::round_price() {
 for (int i = 0; i < company__.totalDays_; i++) {
 company__.price_[i] = round(company__.price_[i], 2);
 }
 }
 
 void MA_GNQTS::Trade::cal_instant_trade_MA() {
 for (int i = 0; i < company__.totalDays_; i++) {
 if (company__.date_[i] == startDate__) {
 startRow__ = i;
 break;
 }
 }
 for (int i = 0; i < company__.totalDays_; i++) {
 if (company__.date_[i] == endDate__) {
 endRow__ = i;
 break;
 }
 }
 if (startRow__ == -1 || endRow__ == -1) {
 cout << "date is out of range" << endl;
 exit(1);
 }
 switch (company__.MAType_[0]) {
 case 'S': {
 for (int dateRow = startRow__ - 1; dateRow <= endRow__; dateRow++) {
 double buyLowMARangePriceSum{0};
 double buyHighMARangePriceSum{0};
 double sellLowMARangePriceSum{0};
 double sellHighMARangePriceSum{0};
 for (int j = buy1__, k = dateRow; j > 0; j--, k--) {
 buyLowMARangePriceSum += company__.price_[k];
 }
 for (int j = buy2__, k = dateRow; j > 0; j--, k--) {
 buyHighMARangePriceSum += company__.price_[k];
 }
 for (int j = sell1__, k = dateRow; j > 0; j--, k--) {
 sellLowMARangePriceSum += company__.price_[k];
 }
 for (int j = sell2__, k = dateRow; j > 0; j--, k--) {
 sellHighMARangePriceSum += company__.price_[k];
 }
 MAbuy1__.push_back(round(buyLowMARangePriceSum / buy1__, 2));
 MAbuy2__.push_back(round(buyHighMARangePriceSum / buy2__, 2));
 MAsell1__.push_back(round(sellLowMARangePriceSum / sell1__, 2));
 MAsell2__.push_back(round(sellHighMARangePriceSum / sell2__, 2));
 }
 
 break;
 }
 case 'W': {
 break;
 }
 case 'E': {
 break;
 }
 }
 }
 
 void MA_GNQTS::Trade::tradeDetail() {
 int stockHold{0};
 double remain{TOTAL_CP_LV};
 tradeRecord__.push_back(",date,price,buy1-1d,buy2-1d,buy1,buy2,sell1-1d, sell2-1d,sell1,sell2,stockHold,remain,capital lv\r");
 for (int i = startRow__, j = 1; i <= endRow__; i++, j++) {
 if (check_buy_cross(stockHold, MAbuy1__[j - 1], MAbuy2__[j - 1], MAbuy1__[j], MAbuy2__[j], i, endRow__) && remain >= price__[i]) {
 stockHold = floor(remain / price__[i]);
 remain = floor(remain - stockHold * price__[i]);
 buyNum__++;
 tradeRecord__.push_back("buy," + date__[i] + "," + to_string(price__[i]) + "," + to_string(MAbuy1__[j - 1]) + "," + to_string(MAbuy2__[j - 1]) + "," + to_string(MAbuy1__[j]) + "," + to_string(MAbuy2__[j]) + ",,,,," + to_string(stockHold) + "," + to_string(remain) + "," + to_string(remain + stockHold * price__[i]) + "\r");
 }
 else if (check_sell_cross(stockHold, MAsell1__[j - 1], MAsell2__[j - 1], MAsell1__[j], MAsell2__[j], i, endRow__)) {
 remain = remain + (double)stockHold * price__[i];
 stockHold = 0;
 sellNum__++;
 tradeRecord__.push_back("sell," + date__[i] + "," + to_string(price__[i]) + ",,,,," + to_string(MAsell1__[j - 1]) + "," + to_string(MAsell2__[j - 1]) + "," + to_string(MAsell1__[j]) + "," + to_string(MAsell2__[j]) + "," + to_string(stockHold) + "," + to_string(remain) + "," + to_string(remain + stockHold * price__[i]) + "\r\r");
 }
 }
 RoR__ = (remain - TOTAL_CP_LV) / TOTAL_CP_LV * 100;
 tradeRecord__.push_back("buyNum," + to_string(buyNum__) + ",sellNum," + to_string(sellNum__) + "\rremain," + to_string(remain) + "\rreturn rate," + to_string(RoR__) + "%\r");
 }
 
 void MA_GNQTS::Trade::outputTradeRecord() {
 ofstream out;
 out.open(company__.companyName_ + "_instantTrade_" + startDate__ + "_" + endDate__ + "_" + to_string(buy1__) + "_" + to_string(buy2__) + "_" + to_string(sell1__) + "_" + to_string(sell2__) + ".csv");
 for_each(tradeRecord__.begin(), tradeRecord__.end(), [&out](auto record) {
 out << record;
 });
 out.close();
 }
 
 MA_GNQTS::Trade::Trade(string startDate, string endDate, int buy1, int buy2, int sell1, int sell2, CompanyInfo &company) : startDate__(startDate), endDate__(endDate), buy1__(buy1), buy2__(buy2), sell1__(sell1), sell2__(sell2), company__(company), date__(company.date_), price__(company.price_) {
 cal_instant_trade_MA();
 round_price();
 tradeDetail();
 outputTradeRecord();
 if (buyNum__ != sellNum__) {
 cout << buyNum__ << "," << sellNum__ << endl;
 cout << "buyNum__ != sellNum__" << endl;
 exit(1);
 }
 }
 
 MA_GNQTS::Trade::Trade(int trainStartRow, int trainEndRow, CompanyInfo::MATable &table, Particle &particle, CompanyInfo &company) : startRow__(trainStartRow), endRow__(trainEndRow), buy1__(particle.buy1_dec__), buy2__(particle.buy2_dec__), sell1__(particle.sell1_dec__), sell2__(particle.sell2_dec__), company__(company), date__(table.tableDate__), price__(table.tablePrice__) {
 for (int i = trainStartRow - 1; i <= trainEndRow; i++) {
 MAbuy1__.push_back(table.MAValues__[i][particle.buy1_dec__]);
 MAbuy2__.push_back(table.MAValues__[i][particle.buy2_dec__]);
 MAsell1__.push_back(table.MAValues__[i][particle.sell1_dec__]);
 MAsell2__.push_back(table.MAValues__[i][particle.sell2_dec__]);
 }
 tradeDetail();
 particle.buyNum__ = buyNum__;
 particle.sellNum__ = sellNum__;
 particle.tradeRecord__ = tradeRecord__;
 //            outputTradeRecord();
 }
 */

/*
 void MA_GNQTS::initialize_local_particles() {
 for (int i = 0; i < PARTICAL_AMOUNT; i++) {
 fill(particles_[i].buy1_bi__.begin(), particles_[i].buy1_bi__.end(), 0);
 fill(particles_[i].buy2_bi__.begin(), particles_[i].buy2_bi__.end(), 0);
 fill(particles_[i].sell1_bi__.begin(), particles_[i].sell1_bi__.end(), 0);
 fill(particles_[i].sell2_bi__.begin(), particles_[i].sell2_bi__.end(), 0);
 particles_[i].buy1_dec__ = 0;
 particles_[i].buy2_dec__ = 0;
 particles_[i].sell1_dec__ = 0;
 particles_[i].sell2_dec__ = 0;
 particles_[i].buyNum__ = 0;
 particles_[i].sellNum__ = 0;
 particles_[i].RoR__ = 0;
 }
 fill(localBest_.buy1_bi__.begin(), localBest_.buy1_bi__.end(), 0);
 fill(localBest_.buy2_bi__.begin(), localBest_.buy2_bi__.end(), 0);
 fill(localBest_.sell1_bi__.begin(), localBest_.sell1_bi__.end(), 0);
 fill(localBest_.sell2_bi__.begin(), localBest_.sell2_bi__.end(), 0);
 localBest_.buy1_dec__ = 0;
 localBest_.buy2_dec__ = 0;
 localBest_.sell1_dec__ = 0;
 localBest_.sell2_dec__ = 0;
 localBest_.buyNum__ = 0;
 localBest_.sellNum__ = 0;
 localBest_.RoR__ = 0;
 
 fill(localWorst_.buy1_bi__.begin(), localWorst_.buy1_bi__.end(), 0);
 fill(localWorst_.buy2_bi__.begin(), localWorst_.buy2_bi__.end(), 0);
 fill(localWorst_.sell1_bi__.begin(), localWorst_.sell1_bi__.end(), 0);
 fill(localWorst_.sell2_bi__.begin(), localWorst_.sell2_bi__.end(), 0);
 localWorst_.buy1_dec__ = 0;
 localWorst_.buy2_dec__ = 0;
 localWorst_.sell1_dec__ = 0;
 localWorst_.sell2_dec__ = 0;
 localWorst_.buyNum__ = 0;
 localWorst_.sellNum__ = 0;
 localWorst_.RoR__ = 1000000;
 }
 */

/*
 void MA_GNQTS::measure(Particle &particle) {
 double r;
 for (int i = 0; i < BUY1_BITS; i++) {
 r = rand();
 r = r / (double)RAND_MAX;
 if (r < betaMtrix_.buy1__[i]) {
 particle.buy1_bi__[i] = 1;
 }
 else {
 particle.buy1_bi__[i] = 0;
 }
 }
 for (int i = 0; i < BUY2_BITS; i++) {
 r = rand();
 r = r / (double)RAND_MAX;
 if (r < betaMtrix_.buy2__[i]) {
 particle.buy2_bi__[i] = 1;
 }
 else {
 particle.buy2_bi__[i] = 0;
 }
 }
 for (int i = 0; i < SELL1_BITS; i++) {
 r = rand();
 r = r / (double)RAND_MAX;
 if (r < betaMtrix_.sell1__[i]) {
 particle.sell1_bi__[i] = 1;
 }
 else {
 particle.sell1_bi__[i] = 0;
 }
 }
 for (int i = 0; i < SELL2_BITS; i++) {
 r = rand();
 r = r / (double)RAND_MAX;
 if (r < betaMtrix_.sell2__[i]) {
 particle.sell2_bi__[i] = 1;
 }
 else {
 particle.sell2_bi__[i] = 0;
 }
 }
 }
 */

/*
 void MA_GNQTS::convert_bi_to_dec(Particle &particle) {
 for (int i = 0, j = BUY1_BITS - 1; i < BUY1_BITS; i++, j--) {
 particle.buy1_dec__ += pow(2, j) * particle.buy1_bi__[i];
 }
 for (int i = 0, j = BUY2_BITS - 1; i < BUY2_BITS; i++, j--) {
 particle.buy2_dec__ += pow(2, j) * particle.buy2_bi__[i];
 }
 for (int i = 0, j = SELL1_BITS - 1; i < SELL1_BITS; i++, j--) {
 particle.sell1_dec__ += pow(2, j) * particle.sell1_bi__[i];
 }
 for (int i = 0, j = SELL2_BITS - 1; i < SELL2_BITS; i++, j--) {
 particle.sell2_dec__ += pow(2, j) * particle.sell2_bi__[i];
 }
 }
 */

/*
 void MA_GNQTS::GNQTStrade(int trainStartRow, int trainEndRow, Particle &particle) {
 int stockHold{0};
 double remain{TOTAL_CP_LV};
 for (int i = trainStartRow; i <= trainEndRow; i++) {
 //        if (stockHold == 0 && table.MAValues__[i - 1][particle.buy1_dec__] <= table.MAValues__[i - 1][particle.buy2_dec__] && table.MAValues__[i][particle.buy1_dec__] > table.MAValues__[i][particle.buy2_dec__] && i != trainEndRow) {
 if (Trade::check_buy_cross(stockHold, MAtable_.MAValues__[i - 1][particle.buy1_dec__], MAtable_.MAValues__[i - 1][particle.buy2_dec__], MAtable_.MAValues__[i][particle.buy1_dec__], MAtable_.MAValues__[i][particle.buy2_dec__], i, trainEndRow) && remain >= MAtable_.tablePrice__[i]) {
 stockHold = floor(remain / MAtable_.tablePrice__[i]);
 remain = remain - stockHold * MAtable_.tablePrice__[i];
 particle.buyNum__++;
 }
 //        else if (stockHold != 0 && ((table.MAValues__[i - 1][particle.sell1_dec__] >= table.MAValues__[i - 1][particle.sell2_dec__] && table.MAValues__[i][particle.sell1_dec__] < table.MAValues__[i][particle.sell2_dec__]) || i == trainEndRow)) {
 else if (Trade::check_sell_cross(stockHold, MAtable_.MAValues__[i - 1][particle.sell1_dec__], MAtable_.MAValues__[i - 1][particle.sell2_dec__], MAtable_.MAValues__[i][particle.sell1_dec__], MAtable_.MAValues__[i][particle.sell2_dec__], i, trainEndRow)) {
 remain = remain + (double)stockHold * MAtable_.tablePrice__[i];
 stockHold = 0;
 particle.sellNum__++;
 }
 }
 if (particle.buyNum__ != particle.sellNum__) {
 cout << particle.buyNum__ << "," << particle.sellNum__ << endl;
 cout << "particle.buyNum__ != particle.sellNum__" << endl;
 exit(1);
 }
 particle.RoR__ = (remain - TOTAL_CP_LV) / TOTAL_CP_LV * 100;
 }
 */
