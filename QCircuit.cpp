#include "QCircuit.h"
#include <algorithm>
#include <windows.h>
#include <fstream>

QCircuit::QCircuit(std::string file_name)
{
    qasm_file_.open(file_name);
    if(!qasm_file_) {
        std::cout << "Unable to open file" << std::endl;
    }
}

bool QCircuit::LevelizeCircuit()
{
    std::string instruction;
    std::string qreg_str;

    while(std::getline(qasm_file_, instruction)) { // parsing the circuit

        if(instruction.find("qreg ") != std::string::npos) { // get the number of registered Qubits
            unsigned found_beg = instruction.find('[');
            unsigned found_end = instruction.find(']');
            qreg_str = instruction.substr(found_beg + 1, found_end - found_beg - 1);
            qreg_ = (unsigned)std::stoi(qreg_str);

            while(std::getline(qasm_file_, instruction)) { // obtaining the Qubits
                int qubit_index_1;
                std::string qubit_index_str_1;

                for(const auto& qgate : qgates_) {

                    if(instruction.find(qgate + ' ') == 0) {// obtain the first Qubit
                        std::vector<std::string> new_level (qreg_, "_");
                        found_beg = instruction.find('[');
                        found_end = instruction.find(']');
                        qubit_index_str_1 = instruction.substr(found_beg + 1, found_end - found_beg - 1);
                        qubit_index_1 = std::stoi(qubit_index_str_1);
                        new_level[qreg_ - qubit_index_1 - 1] = qgate;

                        if(qgate == "cx") { // for CNOT gate, obtain the target Qubit
                            int qubit_index_2;
                            std::string qubit_index_str_2;
                            found_beg = instruction.find('[', found_beg + 1);
                            found_end = instruction.find(']', found_end + 1);
                            qubit_index_str_2 = instruction.substr(found_beg + 1, found_end - found_beg - 1);
                            qubit_index_2 = std::stoi(qubit_index_str_2);
                            new_level[qreg_ - qubit_index_2 - 1] = qgate + "+" + qubit_index_str_1; // target qubit
                            new_level[qreg_ - qubit_index_1 - 1].append("." + qubit_index_str_2);  // controlled qubit
                        }

                        // creating circuit levels
                        if(levels_.empty()) {
                            levels_.push_back(new_level);
                            break;
                        }

                        for(int j = levels_.size() - 1; j >= 0; j--) {
                            std::vector<std::string>& temp_level = levels_[j];
                            bool conflict = false;

                            for (int i = 0; i < qreg_; i++) {
                                if (temp_level[i] != "_" && new_level[i] != "_")
                                    conflict = true;
                            }

                            if(conflict) {
                                if(j == levels_.size()-1) { // conflict and comparing with last level
                                    levels_.push_back(new_level);
                                }
                                else {
                                    for(int i = 0; i < qreg_; i++) { // merge the new level with the previous level
                                        if (new_level[i] != "_")
                                            levels_[j + 1][i] = new_level[i];
                                    }
                                }
                                break;
                            }
                            else if(j == 0){ // not conflict till the beginning
                                for(int i = 0; i < qreg_; i++)
                                        if (new_level[i] != "_")
                                            temp_level[i] = new_level[i];
                            }
                        }
                    }
                }
            }
        }
    }
    levelized_ = true;
    return levelized_;
}

int QCircuit::GetQreg()
{
    return qreg_;
}

int QCircuit::CountLevel()
{
    return levels_.size();
}

std::vector<std::string> QCircuit::GetLevel(int idx)
{
    return levels_[idx];
}

bool QCircuit::TrackSwapping()
{

    if(!levelized_) {
        return false;
    }

    std::vector<int> map_start;
    for(int i = qreg_ - 1; i >= 0; i--) {
        map_start.push_back(i);
    }
    map_.push_back(map_start);

    std::vector< std::vector<std::string> > cp_levels = levels_;

    for(int i = 0; i < cp_levels.size(); i++) { // index of level

        std::vector<std::string> temp_level = cp_levels[i];
        for (int j = 0; j < temp_level.size(); j++) { // index of element of each level

            unsigned pos = temp_level[j].find("cx.");
            if (pos != std::string::npos) {

                int cqubit = j;
                int tqubit = qreg_ - 1 - std::stoi(temp_level[j].substr(pos + 3));

                bool swapped = false;

                if (i < cp_levels.size() - 4) {
                    if (cp_levels[i][cqubit] == cp_levels[i + 2][cqubit] &&
                        cp_levels[i + 2][cqubit] == cp_levels[i + 4][cqubit]) {
                        if (cp_levels[i][tqubit] == cp_levels[i + 2][tqubit] &&
                            cp_levels[i + 2][tqubit] == cp_levels[i + 4][tqubit]) {
                            if (cp_levels[i + 1][cqubit] == cp_levels[i + 3][cqubit] &&
                                cp_levels[i + 3][cqubit] == "h") {
                                if (cp_levels[i + 1][tqubit] == cp_levels[i + 3][tqubit] &&
                                    cp_levels[i + 3][tqubit] == "h") {
                                    swapped = true;

                                    std::string icon = "*";
                                    cp_levels[i][cqubit] = icon;
                                    cp_levels[i + 2][cqubit] = icon;
                                    cp_levels[i + 4][cqubit] = icon;

                                    cp_levels[i][tqubit] = icon;
                                    cp_levels[i + 2][tqubit] = icon;
                                    cp_levels[i + 4][tqubit] = icon;

                                    cp_levels[i + 1][cqubit] = icon;
                                    cp_levels[i + 3][cqubit] = icon;

                                    cp_levels[i + 1][tqubit] = icon;
                                    cp_levels[i + 3][tqubit] = icon;
                                }
                            }
                        }
                    }
                }

                if (swapped) {
                    swapped_location_.push_back(i + 5);
                    std::vector<int> last_map = map_.back();
                    iter_swap(last_map.begin() + cqubit, last_map.begin() + tqubit);
                    map_.push_back(last_map);

                    // delete duplicated swap happened on the same level
                    if(i+5 == swapped_location_[swapped_location_.size()-2]) {
                        swapped_location_.erase (swapped_location_.end()-2);
                        map_.erase (map_.end()-2);
                    }
                }

            }
        }
    }
    mapped_ = true;
    return mapped_;

}

std::vector<int> QCircuit::LocateSwapping()
{
    return swapped_location_;
}

std::vector< std::vector<int> > QCircuit::GetMap()
{
    return map_;
}

std::vector< std::vector< std::vector<bool> > > QCircuit::GetFreePairs()
{

    // construct 3D vector
//    std::vector<int> all_qubits_locations = {13, 12, 11, 10, 9, 8, 7, 6, 5, 1, 3, 2, 4, 0};
    for(int i = 0; i < free_key_qubits_.size(); i++) {

        // update all qubits location
        int map_counter = 0;
        std::vector<int> all_qubits_locations = map_[map_counter];
        std::vector<bool> match (input_qubits_.size(), false); // each match
        std::vector< std::vector<bool> > matches; // match in all levels
        for(int j = 0; j < levels_.size(); j++) {
            if(j == LocateSwapping()[map_counter]) {
//                std::cout << LocateSwapping()[map_counter] << std::endl;
                all_qubits_locations = map_[map_counter];
                map_counter++;
//                std::cout << "Level: " << j << " : ";
//                for(auto L : all_qubits_locations)
//                    std::cout << L << " ";
            }




            for(int k = 0; k < input_qubits_.size(); k++) {

                // find input_qubits_index on each level
                int input_qubits_index = -1;
                for(int location = 0; location < all_qubits_locations.size(); location++) {
                    if(input_qubits_[k] == all_qubits_locations[location]) {

                        input_qubits_index = location;
//                        std::cout << "Looking for " << input_qubits[k] << " " << std::endl;
//                        std::cout << input_qubits_index << " input qb index" << std::endl;
                        break;
                    }
                }

//                std::cout << free_key_qubits[i] << " to " << all_qubits_locations.size() - input_qubits_index - 1<< std::endl;
                if((levels_[j][input_qubits_index] == "_") &&
                (Commutable(free_key_qubits_[i], all_qubits_locations.size() - input_qubits_index - 1)) == 1) {
                    match[k] = true;
                }

            }

            matches.push_back(match);
            match.assign(input_qubits_.size(), false);

        }

        free_pairs_.push_back(matches);

    }

    return free_pairs_;
}

std::vector< std::pair<int, int> > Constrain_14()
{
    // contrain of 14-qubit ibm Melbourne
    std::vector< std::pair<int, int> > constrains;
    std::pair <int, int> couple;

    couple.first = 1; // first is control bit
    couple.second = 0; // second is target bit
    constrains.push_back(couple);

    couple.first = 1;
    couple.second = 2;
    constrains.push_back(couple);

//    couple.first = 3;
//    couple.second = 0;
//    constrains.push_back(couple);

    couple.first = 2;
    couple.second = 3;
    constrains.push_back(couple);

    couple.first = 4;
    couple.second = 3;
    constrains.push_back(couple);

    couple.first = 4;
    couple.second = 10;
    constrains.push_back(couple);

    couple.first = 5;
    couple.second = 4;
    constrains.push_back(couple);

    couple.first = 5;
    couple.second = 6;
    constrains.push_back(couple);

    couple.first = 5;
    couple.second = 9;
    constrains.push_back(couple);

    couple.first = 6;
    couple.second = 8;
    constrains.push_back(couple);

    couple.first = 7;
    couple.second = 8;
    constrains.push_back(couple);

    couple.first = 9;
    couple.second = 8;
    constrains.push_back(couple);

    couple.first = 9;
    couple.second = 10;
    constrains.push_back(couple);

    couple.first = 11;
    couple.second = 10;
    constrains.push_back(couple);

    couple.first = 11;
    couple.second = 12;
    constrains.push_back(couple);

    couple.first = 11;
    couple.second = 3;
    constrains.push_back(couple);

    couple.first = 12;
    couple.second = 2;
    constrains.push_back(couple);

    couple.first = 13;
    couple.second = 1;
    constrains.push_back(couple);

    couple.first = 13;
    couple.second = 12;
    constrains.push_back(couple);

    return constrains;
}

//bool QCircuit::LoadInitialMap(std::string file_name)
//{
//    std::ifstream init_mapping_file;
//    init_mapping_file.open(file_name);
//
//    if (!init_mapping_file) {
//        std::cout << "Unable to read initial mapping file" << std::endl;
//        return false;
//    }
//    else {
//        std::vector<std::string> init_map;
//
//        std::string logical_qbit;
//        std::string arrow;
//        std::string physical_qbit;
//
//        while (true) {
//            if (init_mapping_file.eof())
//                break;
//            init_mapping_file >> logical_qbit >> arrow >> physical_qbit;
//            init_map.push_back(physical_qbit);
//        }
//
//        init_map.pop_back();
//        std::reverse(init_map.begin(), init_map.end());
//
//        return true;
//    }
//}

std::vector< std::pair<int, int> > Constrain_16()
{
    // contrain of 16-qubit ibm
    std::vector< std::pair<int, int> > constrains;
    std::pair <int, int> couple;

    couple.first = 0; // first is control bit
    couple.second = 1; // second is target bit
    constrains.push_back(couple);

    couple.first = 1;
    couple.second = 2;
    constrains.push_back(couple);

    couple.first = 2;
    couple.second = 3;
    constrains.push_back(couple);

    couple.first = 3;
    couple.second = 14;
    constrains.push_back(couple);

    couple.first = 4;
    couple.second = 3;
    constrains.push_back(couple);

    couple.first = 4;
    couple.second = 5;
    constrains.push_back(couple);

    couple.first = 6;
    couple.second = 7;
    constrains.push_back(couple);

    couple.first = 6;
    couple.second = 11;
    constrains.push_back(couple);

    couple.first = 7;
    couple.second = 10;
    constrains.push_back(couple);

    couple.first = 8;
    couple.second = 7;
    constrains.push_back(couple);

    couple.first = 9;
    couple.second = 8;
    constrains.push_back(couple);

    couple.first = 9;
    couple.second = 10;
    constrains.push_back(couple);

    couple.first = 11;
    couple.second = 10;
    constrains.push_back(couple);

    couple.first = 12;
    couple.second = 5;
    constrains.push_back(couple);

    couple.first = 12;
    couple.second = 11;
    constrains.push_back(couple);

    couple.first = 12;
    couple.second = 13;
    constrains.push_back(couple);

    couple.first = 13;
    couple.second = 4;
    constrains.push_back(couple);

    couple.first = 13;
    couple.second = 14;
    constrains.push_back(couple);

    couple.first = 15;
    couple.second = 0;
    constrains.push_back(couple);

    couple.first = 15;
    couple.second = 14;
    constrains.push_back(couple);

    return constrains;
}

int Commutable(int cq, int tq)
{
    // if cq can controls tq, return 1
    std::vector< std::pair <int, int> > constrains = Constrain_14();

    for(auto constrain : constrains) {
        if(cq == constrain.first) {
            if(tq == constrain.second) {
                return 1;
            }
        }
    }

    for(auto constrain : constrains) {
        if(cq == constrain.second) {
            if(tq == constrain.first) {
                return -1;
            }
        }
    }

    return 0;
}

std::vector< std::vector<int> > QCircuit::GetFreePairsResult()
{
    std::vector< std::vector<int> > total_result;
    std::vector<int> free_key_level_result (free_pairs_[0][0].size(), 0);

    for(int i = 0; i < free_pairs_.size(); i++) {
        for (int j = 0; j < free_pairs_[i].size(); j++) {
            for (int k = 0; k < free_pairs_[i][j].size(); k++) {
                if(free_pairs_[i][j][k] == 1) {
                    free_key_level_result[k] += 1;
                }
            }
        }
        total_result.push_back(free_key_level_result);
        free_key_level_result.assign(free_pairs_[0][0].size(), 0);
    }

    return total_result;
}

std::vector< std::pair<int, std::vector<int> > > dijkstra(int source)
{
    // preparation
    unsigned size = 14; // size of the quantum architecture
    int inf = 99; // infinity
    std::vector< std::pair<int, int> > unvisited;
    for(int i = 0; i < size; i++) {
        std::pair<int, int> p = {i, inf}; // first is qubit, second is distance
        unvisited.emplace_back(p);
    }
    unvisited[source].second = 0;
    std::vector< std::pair<int, int> > visited;
    std::vector<int> path;
    path.clear();
    std::vector< std::vector<int> > raw_paths (size, path);
    raw_paths[source].push_back(source);

    // implementation
    while(true) {
        // find minimum weight
        int min = unvisited[0].second;
        int min_pos = 0;
        for(int i = 0; i < unvisited.size(); i++) {
            if(min > unvisited[i].second) {
                min = unvisited[i].second;
                min_pos = i;
            }
        }

        // end loop if nothing left to visit
        if(min == inf) {
            for(auto i : unvisited) {
                visited.push_back(i);
            }
            break;
        }
        else if(unvisited.empty()) {
            break;
        }

        // loop though the unvisited to update the weight
        for(auto& i : unvisited) {
            if (Commutable(unvisited[min_pos].first, i.first) != 0) {
                if (i.second == inf) {
//                    cout << "Comparing " << unvisited[min_pos].first << " with " << i.first << endl;
                    i.second = 0;
                }
                i.second = unvisited[min_pos].second + 1;

                // keep track of paths
                if (unvisited[min_pos].first == source) {
                    raw_paths[i.first].push_back(source);
                    raw_paths[i.first].push_back(i.first);
                } else {
//                    paths[i.first].clear();
                    for(auto j : raw_paths[unvisited[min_pos].first]) {
                        raw_paths[i.first].push_back(j);
                    }
                    raw_paths[i.first].push_back(i.first);
                }
            }

        }
        visited.emplace_back(unvisited[min_pos]);
        unvisited.erase(unvisited.begin() + min_pos);
    }

    std::vector< std::vector<int> > paths (size, path);
    std::vector<std::vector<int> > shortest_paths;
    for(auto i : visited) {
        path.clear();
        path = raw_paths[i.first];

        // divide paths
        std::vector<std::vector<int> > temp_paths;
        temp_paths.clear();
        std::vector<int> temp_path;
        temp_path.clear();
        int goal = path.back();
//        cout << "Checking goal: " << path.back() << endl;
        for (int j = 0; j < path.size();) {
            if (path[j] == source) {
                temp_path.push_back(path[j]);
                j++;
                while (j < path.size() && path[j] != source) {
                    temp_path.push_back(path[j]);
                    j++;
                }
                temp_paths.push_back(temp_path);
                temp_path.clear();
            }
        }

        // complete the paths
        for (int j = 0; j < temp_paths.size(); j++) {
            if (temp_paths[j].back() != goal) {
                temp_paths.erase(temp_paths.begin() + j);
                j = -1;
            }
        }

        // choose the shortest path
        int found_pos = 0;
        for (int j = 0; j < temp_paths.size(); j++) {
            if (temp_paths.size() > 1) {
                int tq = temp_paths[j][temp_paths[j].size() - 1];
                int cq = temp_paths[j][temp_paths[j].size() - 2];
                if (Commutable(cq, tq) == 1) {
                    found_pos = j;
                    break;
                }
                else if ((Commutable(cq, tq) == -1) && (j == temp_paths.size() - 1)) {
                    found_pos = j;
                    break;
                }

            }
            else {
                break;
            }
        }
        temp_path.clear();
        temp_path = temp_paths[found_pos];
        temp_paths.clear();
        temp_paths.push_back(temp_path);
        temp_path.clear();

        shortest_paths.push_back(temp_paths[0]);
    }

//    for (auto k : shortest_paths) {
//        for (auto j : k) {
//            cout << j << " ";
//        }
//        cout << endl;
//    }

    std::vector< std::pair<int, std::vector<int> > > results;
    std::pair<int, std::vector<int> > temp_pair;
    for(auto const& i : shortest_paths) {
        temp_pair.first = 0;
        temp_pair.second = i;
        if(i.size() > 1) {
            temp_pair.first = (i.size() - 2) * 10;

            int tq = i.back();
            int cq = i[i.size() - 2];
            if(Commutable(cq, tq) == -1) {
                temp_pair.first += 2;
            }
        }

        results.push_back(temp_pair);
    }

//    for (auto k : results) {
//            cout << k.first << " ";
//            for(auto z : k.second)
//                cout << z << " ";
//        cout << endl;
//    }

    return results;

}

std::vector< std::vector<int> > ShortestPath()
{
    unsigned size = 14;

    // initialization
    std::vector< std::vector<int> > shortest_path_map (size);
    std::vector<int> ini (size, 0);
    for(int i = 0; i < size; i++) {
        shortest_path_map[i] = ini;
    }

    for(int i = 0; i < size; i++) {
        std::vector< std::pair<int, std::vector<int> > > dij_ret = dijkstra(i);
        for(int j = 0; j < size; j++) {
            int goal = dij_ret[j].second.back();
            shortest_path_map[i][goal] = dij_ret[j].first;
        }
    }

    return shortest_path_map;
}

std::vector< std::vector< std::vector<int> > > QCircuit::GetFreePairsSwap()
{
    std::vector< std::vector<int> > path_map = ShortestPath();



//    for(auto i : free_key_qubits)
//        std::cout << i << " ";

    // construct 3D vector
//    std::vector<int> all_qubits_locations = {13, 12, 11, 10, 9, 8, 7, 6, 5, 1, 3, 2, 4, 0};
    for(int i = 0; i < free_key_qubits_.size(); i++) {

        // update all qubits location
        int map_counter = 0;
        int inf = -1;
        std::vector<int> all_qubits_locations = map_[map_counter];
        std::vector<int> match (input_qubits_.size(), inf); // each match
        std::vector< std::vector<int> > matches; // match in all levels
        for(int j = 0; j < levels_.size(); j++) {
            if(j == LocateSwapping()[map_counter]) {
//                std::cout << LocateSwapping()[map_counter] << std::endl;
                all_qubits_locations = map_[map_counter];
                map_counter++;
//                std::cout << "Level: " << j << " : ";
//                for(auto L : all_qubits_locations)
//                    std::cout << L << " ";
            }
//            std::cout << "ha" << std::endl;
            for(int k = 0; k < input_qubits_.size(); k++) {

                // find input_qubits_index on each level
                int input_qubits_index = -1;
                for(int location = 0; location < all_qubits_locations.size(); location++) {
                    if(input_qubits_[k] == all_qubits_locations[location]) {

                        input_qubits_index = location;
//                        std::cout << "Looking for " << input_qubits[k] << " " << std::endl;
//                        std::cout << input_qubits_index << " input qb index" << std::endl;
                        break;
                    }
                }

//                std::cout << free_key_qubits[i] << " to " << all_qubits_locations.size() - input_qubits_index - 1<< std::endl;
//                if(levels_[j][input_qubits_index] == "*") {
                if(1){
                    int cqubit = free_key_qubits_[i];
                    int tqubit = all_qubits_locations.size() - input_qubits_index - 1;

                    match[k] = path_map[cqubit][tqubit];
//                    if(Commutable(cqubit, tqubit) == -1) {
//                        match[k] = 2;
//                    }
//                    else {
//                        if(path_map[cqubit][tqubit] != inf) {
//                            match[k] = (path_map[cqubit][tqubit] - 1) * 5 * 2;
//                        }
//                    }

                }
            }

            matches.push_back(match);
            match.assign(input_qubits_.size(), inf);

        }

        free_pairs_swap_.push_back(matches);

    }

    return free_pairs_swap_;
}

std::vector< std::vector<int> > QCircuit::GetFreePairsSwapResult()
{
    std::vector< std::vector<int> > total_result;
    std::vector<int> free_key_level_result (free_pairs_swap_[0][0].size(), 0);

    int inf = -1;

    for(int i = 0; i < free_pairs_swap_.size(); i++) {
        for (int j = 0; j < free_pairs_swap_[i].size(); j++) {
            for (int k = 0; k < free_pairs_swap_[i][j].size(); k++) {
                if(free_pairs_swap_[i][j][k] != inf) {
                    free_key_level_result[k] += free_pairs_swap_[i][j][k];
                }
            }
        }
        total_result.push_back(free_key_level_result);
        free_key_level_result.assign(free_pairs_swap_[0][0].size(), 0);
    }

    free_pair_swap_results_ = total_result;
    return total_result;
}

std::vector< std::pair<int, int> > QCircuit::GetCandidates()
{ // sorting the best pairs based on GetFreePairsSwapResult()

    // sorting
    std::vector<std::vector<int> > tmp = free_pair_swap_results_;
    int total_times = tmp.size() * tmp[0].size();
    for(int times = 0; times < total_times; times++) {
        std::pair<int, int> tmp_pair;
        int min = 999999999;
        int min_pos_i = 0;
        int min_pos_j = 0;
        for (int i = 0; i < tmp.size(); i++) {
            for (int j = 0; j < tmp[i].size(); j++) {
                if (min > tmp[i][j]) {
                    min = tmp[i][j];
                    min_pos_i = i;
                    min_pos_j = j;
                }
            }
        }

        tmp_pair.first = free_key_qubits_[min_pos_i];
        tmp_pair.second = input_qubits_[min_pos_j];
        candidates_.push_back(tmp_pair);
        tmp[min_pos_i].erase (tmp[min_pos_i].begin() + min_pos_j);

    }

//    for(auto i : candidates) {
//        std::cout << i.first << " " << i.second << std::endl;
//    }

    return candidates_; // first free key qubit (cqubit), input qubits (second)
}

void QCircuit::LoadCircuitInfo(std::vector<int> input, std::vector<int> output, std::vector<int> rest)
{
    // switch if parameters are wrongly assigned
    output_qubits_ = output;


    input_qubits_ = input;

    // get free key qubits
    for(int i = 0; i < qreg_; i++) {
        bool found = false;
        for(auto j : rest) {
            if(i == j) {
                found = true;
                break;
            }
        }

        if(!found) {
            free_key_qubits_.push_back(i);
        }
    }

    // get aux qubits
    for(int i = 0; i < rest.size(); i++) {
        bool found = false;
        for(int j = 0; j < input_qubits_.size(); j++) {
            if(rest[i] == input_qubits_[j]) {
                found = true;
                break;
            }
        }

        if(!found) {
            aux_qubits_.push_back(rest[i]);
        }
    }

    std::sort (aux_qubits_.begin(), aux_qubits_.end());
    std::sort (input_qubits_.begin(), input_qubits_.end());
    std::sort (output_qubits_.begin(), output_qubits_.end());
    std::sort (free_key_qubits_.begin(), free_key_qubits_.end());
}

std::string SGate(const std::string&  gate_name, const std::string& qubit_name, int qubit)
{
    return gate_name + ' ' + qubit_name + '[' + std::to_string(qubit) + ']' + ';' + '\n';
}

std::string DGate(const std::string&  gate_name,
        const std::string& cqubit_name, int cqubit,
        const std::string& tqubit_name, int tqubit)
{
    return gate_name + ' ' +
    cqubit_name + '[' + std::to_string(cqubit) + ']' + ',' +
    tqubit_name + '[' + std::to_string(tqubit) + ']' + ';' + '\n';
}

std::string SwapGate(const std::string& cqubit_name, int cqubit,
                     const std::string& tqubit_name, int tqubit)
{
    return DGate("cx", cqubit_name, cqubit, tqubit_name, tqubit)+
            SGate("h", cqubit_name, cqubit) +
            SGate("h", tqubit_name, tqubit) +
            DGate("cx", cqubit_name, cqubit, tqubit_name, tqubit) +
            SGate("h", cqubit_name, cqubit) +
            SGate("h", tqubit_name, tqubit) +
            DGate("cx", cqubit_name, cqubit, tqubit_name, tqubit);
}

void QCircuit::AnalysisObfInfo()
{
    std::vector<int> logi_inp = {input_qubits_[1], input_qubits_[2], input_qubits_[0]};
//    std::vector<int> logi_inp = {input_qubits_[0]};
    // manually prioritize inputs based on GetFreePairsSwapResult()


    // all information <log_inp, log_aux, phy_inp, phy_aux, level to swap, level to unswap>
    std::vector< std::vector<int> > all_info;

    // look for input -> aux pairs
    for(int i = 0; i < logi_inp.size(); i++) { // looping though input

        for(int a = 0; a < aux_qubits_.size(); a++) { // looping though aux


            int location_count = 0;
            std::vector<int> current_location = map_[location_count];
            int phy_inp_pos = 0; // physical qubits locations
            int vec_inp_pos = 0; // location in vector

            int phy_aux_pos = 0; // physical qubits locations
            int vec_aux_pos = 0; // location in vector

            for (int j = 0; j < CountLevel(); j++) { // looping though levels


                // update qubit locations
                if (j >= swapped_location_[location_count]) {
                    current_location = map_[location_count];

                    // update input and aux location in the vector
                    for (int g = 0; g < GetLevel(j).size(); g++) {
                        if (current_location[g] == logi_inp[i]) {
                            vec_inp_pos = g;
                            phy_inp_pos = qreg_ - vec_inp_pos - 1; // logical qubit location
                        }

                        if (current_location[g] == aux_qubits_[a]) {
                            vec_aux_pos = g;
                            phy_aux_pos = qreg_ - vec_aux_pos - 1; // logical qubit location
                        }
                    }
                    location_count++;
                }

                // check gates at each level
                std::string gate_str = levels_[j][vec_inp_pos];

                // check whether global swap

                // if inp -> aux (consider if it has no local swap)
                if (gate_str.find("cx.") != std::string::npos) {
//                    std::cout << gate_str << "at level " << j << std::endl;
//                    std::cout << "physical qubit is " << phy_inp_pos << " " ;
//                    std::cout << "vector location is " << vec_inp_pos << " " ;

                    // find target qubit
                    unsigned found_beg = gate_str.find('.');
                    std::string tqubit_str = gate_str.substr(found_beg + 1);
                    int tqubit = std::stoi(tqubit_str);
//                    std::cout << "Found " << tqubit_str << " at level: " << j << std::endl;


                    if(tqubit == phy_aux_pos) { // check whether tqubit == aux

                        // check global swap
                        if(CountLevel() - j > 3) { // make sure not out of scope
                            bool global_swapped = false;
                            if (levels_[j][vec_inp_pos] == levels_[j + 2][vec_inp_pos] &&
                                levels_[j + 2][vec_inp_pos] == levels_[j + 4][vec_inp_pos]) {
                                if (levels_[j][vec_aux_pos] == levels_[j + 2][vec_aux_pos] &&
                                    levels_[j + 2][vec_aux_pos] == levels_[j + 4][vec_aux_pos]) {
                                    if (levels_[j + 1][vec_inp_pos] == levels_[j + 3][vec_inp_pos] &&
                                        levels_[j + 3][vec_inp_pos] == "h") {
                                        if (levels_[j + 1][vec_aux_pos] == levels_[j + 3][vec_aux_pos] &&
                                            levels_[j + 3][vec_aux_pos] == "h") {
                                            global_swapped = true;
                                        }
                                    }
                                }
                            }

                            if (!global_swapped) {
                                // check local swap
                                bool local_swap = false;
                                if (j + 1 < CountLevel() && j > 0) { // in case out of range

                                    // check the h's for next level
                                    if (levels_[j + 1][vec_inp_pos] == "h" && levels_[j + 1][vec_aux_pos] == "h") {
                                        local_swap = true;
                                    }

                                    // check previous h's
                                    if (local_swap) {
                                        local_swap = false;
                                        for (int b = j - 1; b >= 0; b--) { // going back
                                            if (levels_[b][vec_inp_pos] == "h") {
                                                local_swap = true;
                                                break;
                                            }
                                            else if (levels_[b][vec_inp_pos] == "_") {
                                                continue;
                                            }
                                            else {
                                                break;
                                            }
                                        }

                                        if (local_swap) {
                                            local_swap = false;
                                            for (int b = j - 1; b >= 0; b--) { // going back
                                                if (levels_[b][vec_aux_pos] == "h") {
                                                    local_swap = true;
                                                    break;
                                                }
                                                else if (levels_[b][vec_aux_pos] == "_") {
                                                    continue;
                                                }
                                                else {
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                }

                                if (!local_swap) { // confirm it is not a local swap
                                    std::vector<int> info{logi_inp[i], aux_qubits_[a], phy_inp_pos, phy_aux_pos, j,
                                                          j + 1};
                                    all_info.push_back(info);

                                    info.clear();
                                }
                            }
                            else if(global_swapped) {
                                j += 4;
                            }

                        }
                    }

                }

                // if aux -> inp (consider if it has local swap)
                else if (gate_str.find("cx+") != std::string::npos) {

                    // find control qubit
                    unsigned found_beg = gate_str.find('+');
                    std::string cqubit_str = gate_str.substr(found_beg + 1);
                    int cqubit = std::stoi(cqubit_str);

                    if(cqubit == phy_aux_pos) { // check whether tqubit == aux

                        // check global swap
                        if(CountLevel() - j > 3) { // make sure not out of scope
                            bool global_swapped = false;
                            if (levels_[j][vec_inp_pos] == levels_[j + 2][vec_inp_pos] &&
                                levels_[j + 2][vec_inp_pos] == levels_[j + 4][vec_inp_pos]) {
                                if (levels_[j][vec_aux_pos] == levels_[j + 2][vec_aux_pos] &&
                                    levels_[j + 2][vec_aux_pos] == levels_[j + 4][vec_aux_pos]) {
                                    if (levels_[j + 1][vec_inp_pos] == levels_[j + 3][vec_inp_pos] &&
                                        levels_[j + 3][vec_inp_pos] == "h") {
                                        if (levels_[j + 1][vec_aux_pos] == levels_[j + 3][vec_aux_pos] &&
                                            levels_[j + 3][vec_aux_pos] == "h") {
                                            global_swapped = true;
                                        }
                                    }
                                }
                            }

                            if (!global_swapped) {
                                // check local swap
                                bool local_swap = false;
                                int pos_add_swap;
                                if (j + 1 < CountLevel() && j > 0) { // in case out of range

                                    // check the h's for next level
                                    if (levels_[j + 1][vec_inp_pos] == "h" && levels_[j + 1][vec_aux_pos] == "h") {
                                        local_swap = true;
                                    }

                                    // check previous h's
                                    if (local_swap) {
                                        local_swap = false;
                                        for (int b = j - 1; b >= 0; b--) { // going back
                                            if (levels_[b][vec_aux_pos] == "h") {
                                                local_swap = true;
                                                break;
                                            }
                                            else if (levels_[b][vec_aux_pos] == "_") {
                                                continue;
                                            }
                                            else {
                                                break;
                                            }
                                        }

                                        if (local_swap) {
                                            local_swap = false;
                                            for (int b = j - 1; b >= 0; b--) { // going back
                                                if (levels_[b][vec_inp_pos] == "h") {
                                                    pos_add_swap = b;
                                                    local_swap = true;
                                                    break;
                                                }
                                                else if (levels_[b][vec_inp_pos] == "_") {
                                                    continue;
                                                }
                                                else {
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                }

                                if (local_swap) { // confirm it is a local swap
                                    std::vector<int> info{logi_inp[i], aux_qubits_[a], phy_inp_pos, phy_aux_pos, pos_add_swap,
                                                          j + 2};
                                    all_info.push_back(info);
//                                    for(auto z : info) {
//                                        std::cout << z << " " ;
//                                    }
//                                    std::cout << std::endl;
//
//                                    std::cout << "location: ";
//                                    for(auto z : current_location) {
//                                        std::cout << z << " " ;
//                                    }
//                                    std::cout << std::endl;
//                                    std::cout << "Level: " << j << std::endl;
                                    info.clear();
                                }
                            }
                            else if(global_swapped) {
                                j += 4;
                            }
                        }
                    }
                }
            }
        }
    }

    // first process
    int id_cur = 0;
    int id_pre = 0;
    std::vector< std::vector<int> > group_info;
    std::vector< std::vector< std::vector<int> > > all_group_info;
    for(int i = 0; i < all_info.size(); i++) {
        id_cur = all_info[i][1];
        if(id_cur == id_pre) {
            group_info.push_back(all_info[i]);
        }
        else {
            all_group_info.push_back(group_info);
            group_info.clear();
            id_pre = id_cur;
            group_info.push_back(all_info[i]);
        }
    }

    // second process
    group_info.clear();
    std::vector< std::vector< std::vector<int> > > final_group_info;
    for(int i = 0; i < all_group_info.size(); i++) {
        for(int j = 0; j < all_group_info[i].size()/4; j++) {
            for(int k = j; k < all_group_info[i].size(); k+=all_group_info[i].size()/4) {
                group_info.push_back(all_group_info[i][k]);
            }
            final_group_info.push_back(group_info);
            group_info.clear();
        }
    }

    all_obf_info_ = final_group_info;
//    for(auto i : all_obf_info_[0]) {
//        for(auto j : i) {
//            std::cout << j << " ";
//        }
//        std::cout << std::endl;
//    }
//    std::cout << std::endl;


}

//void QCircuit::Obfuscate(int x)
//{
//    int kqubit = 10; // manually choose the 10th physical qubit as key
//
//    std::string path = "C:\\Users\\bo\\Desktop\\IBMQCPP\\grover\\ObfuscatedQasm\\out1.qasm";
//    std::ofstream outfile (path);
//
//    outfile << "OPENQASM 2.0;\n";
//    outfile << "include \"qelib1.inc\";\n";
//    outfile << "qreg q[14];\n";
//    outfile << "creg c[3];\n";
//    outfile << "qreg key[1];\n";
//
//
//    int times_left = x;
//    int counter = 0;
//    int identifier_pre = -1;
//    int identifier_curr = 0;
//    for(int i = 0; i < all_obf_info_.size(); i++) {
//        identifier_curr = all_obf_info_[i][2];
//
//
//        if(times_left >= 0) {
//            if (identifier_curr == identifier_pre) {
//
//
//                int location_count = 0;
//                for(int j = 0; j < CountLevel(); j++) { // levels
//                    for(int k = 0; k < levels_[j].size(); k++) { // gates
//
//                        // identify gate name
//                        if(levels_[j][k] != "_") {
//                            std::string gate_name;
//                            for (int g = 0; g < qgates_.size(); g++) {
//                                if(levels_[j][k].find("cx+") != std::string::npos) {
//                                    gate_name = "cx+";
//                                    break;
//                                }
//                                else if(levels_[j][k].find("cx.") != std::string::npos) {
//                                    gate_name = "cx";
//                                    break;
//                                }
//                                else if (levels_[j][k] == qgates_[g]) {
//                                    gate_name = qgates_[g];
//                                    break;
//                                }
//                            }
//
//                            // insert swap
//                            bool swap = false;
//                            std::vector<int> path;
//                            if(gate_name == "cx" && j == all_obf_info_[location_count][0]) {
//                                // find the path key -> input
//                                std::vector< std::pair<int, std::vector<int> > > path_result = dijkstra(kqubit);
//                                for(int d = 0; d < path_result.size(); d++) {
//                                    for(int v = 0; v < path_result[d].second.size(); v++) {
//                                        if(path_result[d].second.back() == all_obf_info_[i][3]) {
//                                            path = path_result[d].second;
//                                        }
//                                    }
//                                }
//
//
//                                // insert swap gates
//                                if(path.size() > 2) {
//                                    for (int s = 0; s < path.size() - 2; s++) {
//
//                                        // identify qubit names
//                                        std::string qb_name_1;
//                                        int k_qb = 0;
//                                        int qb_1 = path[s];
//                                        if (path[s] == kqubit) {
//                                            qb_name_1 = "key";
//                                        }
//                                        else
//                                            qb_name_1 = "q";
//
//                                        std::string qb_name_2;
//                                        int qb_2 = path[s + 1];
//                                        if (path[s + 1] == kqubit)
//                                            qb_name_2 = "key";
//                                        else
//                                            qb_name_2 = "q";
//
//                                        // check constrain and add swap gates
//                                        if (Commutable(qb_1, qb_2) == 1) {
//                                            if(qb_name_1 == "key") {
//                                                outfile << SwapGate(qb_name_1, k_qb, qb_name_2, qb_2);
//                                            }
//                                            else if(qb_name_2 == "key") {
//                                                outfile << SwapGate(qb_name_1, qb_1, qb_name_2, k_qb);
//                                            }
//                                            else {
//                                                outfile << SwapGate(qb_name_1, qb_1, qb_name_2, qb_2);
//                                            }
//                                        }
//                                        else {
//                                            if(qb_name_1 == "key") {
//                                                outfile << SwapGate(qb_name_2, qb_2, qb_name_1, k_qb);
//                                            }
//                                            else if(qb_name_2 == "key") {
//                                                outfile << SwapGate(qb_name_2, k_qb, qb_name_1, qb_1);
//                                            }
//                                            else {
//                                                outfile << SwapGate(qb_name_2, qb_2, qb_name_1, qb_1);
//                                            }
//                                        }
//
//                                    }
//
//                                    swap = true;
//                                }
//                                if((!path.empty() && path.size() < 2) || swap) {
//                                    // identify qubit names
//                                    std::string qb_name_1;
//                                    int k_qb = 0;
//                                    int qb_1 = path[path.size()-2];
//                                    if (path[path.size()-2] == kqubit)
//                                        qb_name_1 = "key";
//                                    else
//                                        qb_name_1 = "q";
//
//                                    std::string qb_name_2;
//                                    int qb_2 = path[path.size()-1];
//                                    if (path[path.size()-1] == kqubit)
//                                        qb_name_2 = "key";
//                                    else
//                                        qb_name_2 = "q";
//
//                                    if (Commutable(qb_1, qb_2) == 1) {
//                                        if(qb_name_1 == "key") {
//                                            outfile << DGate("cx", qb_name_1, k_qb, qb_name_2, qb_2);
//                                        }
//                                        else if(qb_name_2 == "key") {
//                                            outfile << DGate("cx", qb_name_1, qb_1, qb_name_2, k_qb);
//                                        }
//                                        else {
//                                            outfile << DGate("cx", qb_name_1, qb_1, qb_name_2, qb_2);
//                                        }
//                                    }
//                                    else {
//                                        if(qb_name_1 == "key") {
//                                            outfile << SGate("h", qb_name_1, k_qb);
//                                            outfile << SGate("h", qb_name_2, qb_2);
//                                            outfile << DGate("cx", qb_name_2, qb_2, qb_name_1, k_qb);
//                                            outfile << SGate("h", qb_name_1, k_qb);
//                                            outfile << SGate("h", qb_name_2, qb_2);
//                                        }
//                                        else if(qb_name_2 == "key") {
//                                            outfile << SGate("h", qb_name_1, qb_1);
//                                            outfile << SGate("h", qb_name_2, k_qb);
//                                            outfile << DGate("cx", qb_name_2, k_qb, qb_name_1, qb_1);
//                                            outfile << SGate("h", qb_name_1, qb_1);
//                                            outfile << SGate("h", qb_name_2, k_qb);
//                                        }
//                                        else {
//                                            outfile << SGate("h", qb_name_1, qb_1);
//                                            outfile << SGate("h", qb_name_2, qb_2);
//                                            outfile << DGate("cx", qb_name_2, qb_2, qb_name_1, qb_1);
//                                            outfile << SGate("h", qb_name_1, qb_1);
//                                            outfile << SGate("h", qb_name_2, qb_2);
//                                        }
//
//                                    }
//                                }
//                            }
//
//                            if(swap) {
//                                for(auto z : path) {
//                                    std::cout << z << " ";
//                                }
//                                std::cout << std::endl;
//                            }
//
//
//                            // identify physical location
//                            if (gate_name != "cx" && gate_name != "cx+") {
//                                int phy_location = qreg_ - k - 1;
//                                outfile << SGate(gate_name, "q", phy_location);
//                            }
//                            if (gate_name == "cx") {
//                                int cq_phy_location = qreg_ - k - 1;
//
//                                unsigned found_beg = levels_[j][k].find('.');
//                                std::string tqubit_str = levels_[j][k].substr(found_beg + 1);
//                                int tq_phy_location = std::stoi(tqubit_str);
//
//                                outfile << DGate(gate_name, "q", cq_phy_location, "q", tq_phy_location);
//                            }
//
//
//                            // uncompute the swap gates
//                            if((!path.empty() && path.size() < 2) || swap) {
//                                // identify qubit names
//                                std::string qb_name_1;
//                                int k_qb = 0;
//                                int qb_1 = path[path.size()-2];
//                                if (path[path.size()-2] == kqubit)
//                                    qb_name_1 = "key";
//                                else
//                                    qb_name_1 = "q";
//
//                                std::string qb_name_2;
//                                int qb_2 = path[path.size()-1];
//                                if (path[path.size()-1] == kqubit)
//                                    qb_name_2 = "key";
//                                else
//                                    qb_name_2 = "q";
//
//                                if (Commutable(qb_1, qb_2) == 1) {
//                                    if(qb_name_1 == "key") {
//                                        outfile << DGate("cx", qb_name_1, k_qb, qb_name_2, qb_2);
//                                    }
//                                    else if(qb_name_2 == "key") {
//                                        outfile << DGate("cx", qb_name_1, qb_1, qb_name_2, k_qb);
//                                    }
//                                    else {
//                                        outfile << DGate("cx", qb_name_1, qb_1, qb_name_2, qb_2);
//                                    }
//                                }
//                                else {
//                                    if (qb_name_1 == "key") {
//                                        outfile << SGate("h", qb_name_1, k_qb);
//                                        outfile << SGate("h", qb_name_2, qb_2);
//                                        outfile << DGate("cx", qb_name_2, qb_2, qb_name_1, k_qb);
//                                        outfile << SGate("h", qb_name_1, k_qb);
//                                        outfile << SGate("h", qb_name_2, qb_2);
//                                    } else if (qb_name_2 == "key") {
//                                        outfile << SGate("h", qb_name_1, qb_1);
//                                        outfile << SGate("h", qb_name_2, k_qb);
//                                        outfile << DGate("cx", qb_name_2, k_qb, qb_name_1, qb_1);
//                                        outfile << SGate("h", qb_name_1, qb_1);
//                                        outfile << SGate("h", qb_name_2, k_qb);
//                                    } else {
//                                        outfile << SGate("h", qb_name_1, qb_1);
//                                        outfile << SGate("h", qb_name_2, qb_2);
//                                        outfile << DGate("cx", qb_name_2, qb_2, qb_name_1, qb_1);
//                                        outfile << SGate("h", qb_name_1, qb_1);
//                                        outfile << SGate("h", qb_name_2, qb_2);
//                                    }
//                                }
//                            }
//
//                            if(swap) {
//                                for (int s = path.size() - 3; s >= 0; s--) {
//                                    // identify qubit names
//                                    std::string qb_name_1;
//                                    int k_qb = 0;
//                                    int qb_1 = path[s];
//                                    if (path[s] == kqubit)
//                                        qb_name_1 = "key";
//                                    else
//                                        qb_name_1 = "q";
//
//                                    std::string qb_name_2;
//                                    int qb_2 = path[s + 1];
//                                    if (path[s + 1] == kqubit)
//                                        qb_name_2 = "key";
//                                    else
//                                        qb_name_2 = "q";
//
//                                    // check constrain and add swap gates
//                                    if (Commutable(qb_1, qb_2) == 1) {
//                                        if(qb_name_1 == "key") {
//                                            outfile << SwapGate(qb_name_1, k_qb, qb_name_2, qb_2);
//                                        }
//                                        else if(qb_name_2 == "key") {
//                                            outfile << SwapGate(qb_name_1, qb_1, qb_name_2, k_qb);
//                                        }
//                                        else {
//                                            outfile << SwapGate(qb_name_1, qb_1, qb_name_2, qb_2);
//                                        }
//                                    }
//                                    else {
//                                        if(qb_name_1 == "key") {
//                                            outfile << SwapGate(qb_name_2, qb_2, qb_name_1, k_qb);
//                                        }
//                                        else if(qb_name_2 == "key") {
//                                            outfile << SwapGate(qb_name_2, k_qb, qb_name_1, qb_1);
//                                        }
//                                        else {
//                                            outfile << SwapGate(qb_name_2, qb_2, qb_name_1, qb_1);
//                                        }
//
//                                    }
//                                }
//                            }
//                        }
//                    }
//                }
//
//
//            }
//            else {
//                identifier_pre = identifier_curr;
//                times_left--;
//                i--;
//                counter++;
//            }
//        }
//    }
//
//    outfile.close();
//}

std::vector< std::vector< std::vector<int> > > QCircuit::GetObfInfo()
{
    return all_obf_info_;
}

bool sortcol( const std::vector<int>& v1,
              const std::vector<int>& v2 ) {
    return v1[4] < v2[4];
}

void QCircuit::Obfuscate(int x) {
    int kqubit = 10; // manually choose the 10th physical qubit as key
    std::string dir = "C:\\Users\\phi\\Desktop\\IBMQCPP\\grover\\ObfuscatedQasm\\out1.qasm";
    std::ofstream outfile (dir);

    outfile << "OPENQASM 2.0;\n";
    outfile << "include \"qelib1.inc\";\n";
    outfile << "qreg q[14];\n";
    outfile << "creg c[3];\n";
    outfile << "qreg key[1];\n";

    std::vector< std::vector< std::vector<int> > > all_obf_info = all_obf_info_;


    std::vector< std::vector<int> > needed_obf_gates_info;
//    for(int i = 0; i < x; i++) {
//        for(int j = 0; j < all_obf_info[i].size(); j++) {
//            needed_obf_gates_info.push_back(all_obf_info[i][j]);
//        }
//    }

//    needed_obf_gates_info = all_obf_info_[4];

    needed_obf_gates_info.push_back(all_obf_info_[4][0]);
    needed_obf_gates_info.push_back(all_obf_info_[4][1]);
    needed_obf_gates_info.push_back(all_obf_info_[4][2]);
    needed_obf_gates_info.push_back(all_obf_info_[4][3]);

    needed_obf_gates_info.push_back(all_obf_info_[0][0]);
    needed_obf_gates_info.push_back(all_obf_info_[0][1]);
    needed_obf_gates_info.push_back(all_obf_info_[0][2]);
    needed_obf_gates_info.push_back(all_obf_info_[0][3]);

    needed_obf_gates_info.push_back(all_obf_info_[10][0]);
    needed_obf_gates_info.push_back(all_obf_info_[10][1]);
    needed_obf_gates_info.push_back(all_obf_info_[10][2]);
    needed_obf_gates_info.push_back(all_obf_info_[10][3]);

    needed_obf_gates_info.push_back(all_obf_info_[12][0]);
    needed_obf_gates_info.push_back(all_obf_info_[12][1]);
    needed_obf_gates_info.push_back(all_obf_info_[12][2]);
    needed_obf_gates_info.push_back(all_obf_info_[12][3]);

    needed_obf_gates_info.push_back(all_obf_info_[14][0]);
    needed_obf_gates_info.push_back(all_obf_info_[14][1]);
    needed_obf_gates_info.push_back(all_obf_info_[14][2]);
    needed_obf_gates_info.push_back(all_obf_info_[14][3]);

    sort(needed_obf_gates_info.begin(), needed_obf_gates_info.end(),sortcol);

    std::vector< std::vector<int> > aaaa = needed_obf_gates_info;

    for(auto i : aaaa) {
        for(auto j : i) {
            std::cout << j << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;


    int obf_location_count = 0;
    std::vector<int> obf_info = needed_obf_gates_info[obf_location_count];
    bool swapped = false;
    std::vector<int> path;
    for(int i = 0; i < CountLevel(); i++) { // levels

        // load obf_info
        if(!obf_info.empty() && obf_info[4] == i) { // found insert swap level
            swapped = true;

            // find the path key -> input
            std::vector< std::pair<int, std::vector<int> > > path_result = dijkstra(kqubit);
            for(int d = 0; d < path_result.size(); d++) {
                for(int v = 0; v < path_result[d].second.size(); v++) {
                    if(path_result[d].second.back() == obf_info[2]) {
                        path = path_result[d].second;
                    }
                }
            }
//            for(auto z : path) {
//                std::cout << z << " ";
//            }
//            std::cout << std::endl;
        }


        // insert swap
        if(swapped && obf_info[4] == i) {
            if (path.size() > 2) {
                for (int s = 0; s < path.size() - 2; s++) {

                    // identify qubit names
                    std::string qb_name_1;
                    int k_qb = 0;
                    int qb_1 = path[s];
                    if (path[s] == kqubit) {
                        qb_name_1 = "key";
                    } else
                        qb_name_1 = "q";

                    std::string qb_name_2;
                    int qb_2 = path[s + 1];
                    if (path[s + 1] == kqubit)
                        qb_name_2 = "key";
                    else
                        qb_name_2 = "q";

                    // check constrain and add swap gates
                    if (Commutable(qb_1, qb_2) == 1) {
                        if (qb_name_1 == "key") {
                            outfile << SwapGate(qb_name_1, k_qb, qb_name_2, qb_2);
                        } else if (qb_name_2 == "key") {
                            outfile << SwapGate(qb_name_1, qb_1, qb_name_2, k_qb);
                        } else {
                            outfile << SwapGate(qb_name_1, qb_1, qb_name_2, qb_2);
                        }
                    } else {
                        if (qb_name_1 == "key") {
                            outfile << SwapGate(qb_name_2, qb_2, qb_name_1, k_qb);
                        } else if (qb_name_2 == "key") {
                            outfile << SwapGate(qb_name_2, k_qb, qb_name_1, qb_1);
                        } else {
                            outfile << SwapGate(qb_name_2, qb_2, qb_name_1, qb_1);
                        }
                    }
                }
            }

            // insert key gate
                // identify qubit names
            std::string qb_name_1;
            int k_qb = 0;
            int qb_1 = path[path.size() - 2];
            if (path[path.size() - 2] == kqubit)
                qb_name_1 = "key";
            else
                qb_name_1 = "q";

            std::string qb_name_2;
            int qb_2 = path[path.size() - 1];
            if (path[path.size() - 1] == kqubit)
                qb_name_2 = "key";
            else
                qb_name_2 = "q";

            if (Commutable(qb_1, qb_2) == 1) {
                if (qb_name_1 == "key") {
                    outfile << DGate("cx", qb_name_1, k_qb, qb_name_2, qb_2);
                } else if (qb_name_2 == "key") {
                    outfile << DGate("cx", qb_name_1, qb_1, qb_name_2, k_qb);
                } else {
                    outfile << DGate("cx", qb_name_1, qb_1, qb_name_2, qb_2);
                }
            } else {
                if (qb_name_1 == "key") {
                    outfile << SGate("h", qb_name_1, k_qb);
                    outfile << SGate("h", qb_name_2, qb_2);
                    outfile << DGate("cx", qb_name_2, qb_2, qb_name_1, k_qb);
                    outfile << SGate("h", qb_name_1, k_qb);
                    outfile << SGate("h", qb_name_2, qb_2);
                } else if (qb_name_2 == "key") {
                    outfile << SGate("h", qb_name_1, qb_1);
                    outfile << SGate("h", qb_name_2, k_qb);
                    outfile << DGate("cx", qb_name_2, k_qb, qb_name_1, qb_1);
                    outfile << SGate("h", qb_name_1, qb_1);
                    outfile << SGate("h", qb_name_2, k_qb);
                } else {
                    outfile << SGate("h", qb_name_1, qb_1);
                    outfile << SGate("h", qb_name_2, qb_2);
                    outfile << DGate("cx", qb_name_2, qb_2, qb_name_1, qb_1);
                    outfile << SGate("h", qb_name_1, qb_1);
                    outfile << SGate("h", qb_name_2, qb_2);
                }

            }
        }





        // uncompute swap
        bool uncomputed = false;
        if(swapped && obf_info[5] == i) {
            std::cout << "LEvel: " << i << std::endl;

            std::cout << "OBF_info:" << std::endl;
            for(auto z : obf_info) {
                std::cout << z << " ";
            }
            std::cout << std::endl;

            std::cout << "path: " << std::endl;
            for(auto z : path) {
                std::cout << z << " ";
            }
            std::cout << std::endl;


            std::cout << std::endl;

//            if ((!path.empty() && path.size() < 2)) {

            // identify qubit names
            std::string qb_name_1;
            int k_qb = 0;
//            std::cout << path.size() << std::endl;
            int qb_1 = path[path.size()-2];
            if (path[path.size()-2] == kqubit)
                qb_name_1 = "key";
            else
                qb_name_1 = "q";

            std::string qb_name_2;
            int qb_2 = path[path.size()-1];
            if (path[path.size()-1] == kqubit)
                qb_name_2 = "key";
            else
                qb_name_2 = "q";


            if (Commutable(qb_1, qb_2) == 1) {
                if(qb_name_1 == "key") {
                    outfile << DGate("cx", qb_name_1, k_qb, qb_name_2, qb_2);
                }
                else if(qb_name_2 == "key") {
                    outfile << DGate("cx", qb_name_1, qb_1, qb_name_2, k_qb);
                }
                else {
                    outfile << DGate("cx", qb_name_1, qb_1, qb_name_2, qb_2);
                }
            }
            else {
                if (qb_name_1 == "key") {
                    outfile << SGate("h", qb_name_1, k_qb);
                    outfile << SGate("h", qb_name_2, qb_2);
                    outfile << DGate("cx", qb_name_2, qb_2, qb_name_1, k_qb);
                    outfile << SGate("h", qb_name_1, k_qb);
                    outfile << SGate("h", qb_name_2, qb_2);
                } else if (qb_name_2 == "key") {
                    outfile << SGate("h", qb_name_1, qb_1);
                    outfile << SGate("h", qb_name_2, k_qb);
                    outfile << DGate("cx", qb_name_2, k_qb, qb_name_1, qb_1);
                    outfile << SGate("h", qb_name_1, qb_1);
                    outfile << SGate("h", qb_name_2, k_qb);
                } else {
                    outfile << SGate("h", qb_name_1, qb_1);
                    outfile << SGate("h", qb_name_2, qb_2);
                    outfile << DGate("cx", qb_name_2, qb_2, qb_name_1, qb_1);
                    outfile << SGate("h", qb_name_1, qb_1);
                    outfile << SGate("h", qb_name_2, qb_2);
                }
            }

            for (int s = path.size() - 3; s >= 0; s--) {
                // identify qubit names
//                std::string qb_name_1;
                k_qb = 0;
                qb_1 = path[s];
                if (path[s] == kqubit)
                    qb_name_1 = "key";
                else
                    qb_name_1 = "q";

//                std::string qb_name_2;
                qb_2 = path[s + 1];
                if (path[s + 1] == kqubit)
                    qb_name_2 = "key";
                else
                    qb_name_2 = "q";

                // check constrain and add swap gates
                if (Commutable(qb_1, qb_2) == 1) {
                    if(qb_name_1 == "key") {
                        outfile << SwapGate(qb_name_1, k_qb, qb_name_2, qb_2);
                    }
                    else if(qb_name_2 == "key") {
                        outfile << SwapGate(qb_name_1, qb_1, qb_name_2, k_qb);
                    }
                    else {
                        outfile << SwapGate(qb_name_1, qb_1, qb_name_2, qb_2);
                    }
                }
                else {
                    if(qb_name_1 == "key") {
                        outfile << SwapGate(qb_name_2, qb_2, qb_name_1, k_qb);
                    }
                    else if(qb_name_2 == "key") {
                        outfile << SwapGate(qb_name_2, k_qb, qb_name_1, qb_1);
                    }
                    else {
                        outfile << SwapGate(qb_name_2, qb_2, qb_name_1, qb_1);
                    }
                }
            }


            if (obf_location_count < needed_obf_gates_info.size() - 1) {
                obf_location_count++;
                obf_info = needed_obf_gates_info[obf_location_count];
            }
            swapped = false;
            uncomputed = true;

        }


        if(uncomputed && obf_info[4] == i) {
            i--;
            uncomputed = false;
        }
        else {
            for(int j = 0; j < levels_[i].size(); j++) { // gates

                // identify gate name
                std::string gate_name;
                gate_name.clear();
                if(levels_[i][j] != "_" && levels_[i][j].find("cx+") == std::string::npos) {
                    for (int g = 0; g < qgates_.size(); g++) {
                        if (levels_[i][j].find("cx.") != std::string::npos) {
                            gate_name = "cx";
                            break;
                        }
                        else if (levels_[i][j] == qgates_[g]) {
                            gate_name = qgates_[g];
                            break;
                        }
                    }
                }

                if(!gate_name.empty()) {
                    // inserting gates in the original circuit
                    if (gate_name != "cx") {
                        int cq_phy_location = qreg_ - j - 1;
                        outfile << SGate(gate_name, "q", cq_phy_location);
                    }
                    else if (gate_name == "cx") {
                        int cq_phy_location = qreg_ - j - 1;

                        unsigned found_beg = levels_[i][j].find('.');
                        std::string tqubit_str = levels_[i][j].substr(found_beg + 1);
                        int tq_phy_location = std::stoi(tqubit_str);

                        outfile << DGate(gate_name, "q", cq_phy_location, "q", tq_phy_location);
                    }
                }

            }
        }





    }

    outfile << "measure q[13] -> c[2];" << std::endl;
    outfile << "measure q[1] -> c[1];" << std::endl;
    outfile << "measure q[2] -> c[0];" << std::endl;

    outfile.close();
}