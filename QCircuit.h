#ifndef IBMQCPP_QCIRCUIT_H
#define IBMQCPP_QCIRCUIT_H

#include <fstream>
#include <iostream>
#include <vector>
#include <array>

class QCircuit {
 public:
    explicit QCircuit(std::string);
    bool LevelizeCircuit();
    bool TrackSwapping(); // takes initial mapping file as parameter
    std::vector<int> LocateSwapping();
    int CountLevel(); // return the number of total levels
    std::vector<std::string> GetLevel(int); // return certain level in boolean vector
    int GetQreg(); // return the number of registered Qubits
    std::vector< std::vector<int> > GetMap();
    std::vector< std::vector< std::vector<bool> > > GetFreePairs();
    std::vector< std::vector<int> > GetFreePairsResult();
    std::vector< std::vector< std::vector<int> > > GetFreePairsSwap();
    std::vector< std::vector<int> > GetFreePairsSwapResult();
    void LoadCircuitInfo(std::vector<int>, std::vector<int>, std::vector<int>);
    std::vector< std::pair<int, int> > GetCandidates();
    void AnalysisObfInfo();
    std::vector< std::vector< std::vector<int> > > GetObfInfo();
    void Obfuscate (int);


 private:
    bool mapped_ = false;
    bool levelized_ = false;
    std::vector<int> swapped_location_ {0}; // store the location(level) of two qubits where finish swapping
    unsigned qreg_ = 0;
    std::ifstream qasm_file_;
    std::vector< std::vector<int> > map_; // swapped map
    std::vector< std::vector<std::string> > levels_;
    std::array<std::string,13> qgates_ = {"x", "y", "z", "h", "s", "sdg", "cx", "t", "tdg", "id", "u1", "u2", "u3"};
    std::vector< std::vector< std::vector<bool> > > free_pairs_;
    std::vector< std::vector< std::vector<int> > > free_pairs_swap_;
    std::vector<int> input_qubits_;
    std::vector<int> output_qubits_;
    std::vector<int> free_key_qubits_;
    std::vector<int> aux_qubits_;
    std::vector< std::vector<int> > free_pair_swap_results_;
    std::vector< std::pair<int, int> > candidates_;
    std::vector< std::vector< std::vector<int> > > all_obf_info_;
};

int Commutable(int, int);
std::vector< std::vector<int> > ShortestPath();
std::vector< std::pair<int, std::vector<int> > > dijkstra(int);
std::string SGate(const std::string&  gate_name, const std::string& qubit_name, int qubit);
std::string DGate(const std::string&  gate_name,
                  const std::string& cqubit_name, int cqubit,
                  const std::string& tqubit_name, int tqubit);
std::string SwapGate(const std::string& cqubit_name, int cqubit,
                     const std::string& tqubit_name, int tqubit);
void CreateDir (std::string dirname);
#endif //IBMQCPP_QCIRCUIT_H
