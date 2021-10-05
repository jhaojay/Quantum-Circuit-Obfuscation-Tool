#include <iostream>
#include <fstream>
#include "QCircuit.h"
#include <algorithm>
using namespace std;



int main()
{

//    for(auto i : dijkstra(12)) {
//        cout << i.first << " ";
//        for(auto j : i.second) {
//            cout << j << " ";
//        }
//        cout << endl;
//    }



    QCircuit cir2("C:\\Users\\phi\\Desktop\\IBMQCPP\\grover\\output.qasm");
    cir2.LevelizeCircuit();
    cir2.TrackSwapping();

//    cout << cir2.CountLevel() << endl;
//    for(int i = 0; i < cir2.GetMap().size(); i++)
//    {
//        cout << cir2.LocateSwapping()[i] << ": ";
//        for(int j = 0; j < cir2.GetMap()[i].size(); j++) {
//            std::cout << cir2.GetMap()[i][j] << " ";
//        }
//        std::cout << std::endl;
//    }
    vector<int> input = {1,2,13};
    vector<int> output = {4};
    vector<int> rest = {0,12,11,3};
//     free: 5, 6, 7, 8, 9, 10

    cir2.LoadCircuitInfo(input, output, rest);
    cir2.AnalysisObfInfo();
    std::vector< std::vector< std::vector<int> > > aaaa = cir2.GetObfInfo();

//    for(auto i : aaaa[16]) {
//        for(auto j : i) {
//            std::cout << j << " ";
//        }
//        std::cout << std::endl;
//    }
//    std::cout << std::endl;

    cir2.Obfuscate(1);


//    std::vector< std::vector< std::vector<int> > > free_pairs = cir2.GetFreePairsSwap();

//    cir2.Obfuscate(1);

//    for(int i = 0; i < cir2.CountLevel(); i++) {
//        cout << "Level:" << i << "  ";
//        for(int j = 0; j < free_pairs[5][i].size(); j++) {
//            cout << free_pairs[5][i][j] << " ";
//        }
//        cout << endl;
//    }
//
//    for(int i = 0; i < cir2.CountLevel(); i++) {
//        cout << "Level: " << i << " ";
//        for(auto j : cir2.GetLevel(i)) {
//            cout << j << " ";
//        }
//        cout << endl;
//    }

//    cout << endl;
//    for(auto i : cir2.GetFreePairsSwapResult()) {
//        for(auto j : i) {
//            cout << j << " ";
//        }
//        cout << endl;
//    }


//    for(auto i : dijkstra(10)) {
//        cout << i.first << " ";
//        for(auto j : i.second) {
//            cout << j << " " ;
//        }
//        cout << endl;
//    }


    return 0;
}
