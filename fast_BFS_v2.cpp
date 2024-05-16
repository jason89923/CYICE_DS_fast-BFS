/* Copyright 2024.5.16 by Jason HO.  All rights reserved. */

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

// pre-defined structure
struct Connection {
    char putID[12];
    char getID[12];
    float weight;
};

struct Node {
    string getID;
    float weight;
};

struct AdjacencyListNode {
    string putID;
    vector<Node> connections;

    AdjacencyListNode(const Connection& connection) {
        putID = connection.putID;
        connections.push_back(Node{connection.getID, connection.weight});
    }

    void sort_connections() {
        sort(connections.begin(), connections.end(), [](const Node& a, const Node& b) {
            return a.getID < b.getID;
        });
    }
};

struct BFSNode {
    string putID;
    set<string> connections;
};

class AdjacencyList {
   private:
    vector<AdjacencyListNode> adjacencyList;  // adjacency list
    unordered_map<string, int> nodeIndex;     // node index
    vector<BFSNode> BFS_result;               // BFS result
    unordered_map<string, int> BFS_index;     // BFS index

    unordered_map<string, set<string>> index;           // inverted index
    unordered_map<string, set<string>> inverted_index;  // inverted index

    unordered_map<string, unordered_set<string>> dual_side_index;  // dual side index

   public:
    void clear() {
        adjacencyList.clear();
        nodeIndex.clear();
        index.clear();
        inverted_index.clear();
        dual_side_index.clear();
        BFS_result.clear();
        BFS_index.clear();
    }

    void build(fstream& file) {
        auto start = chrono::high_resolution_clock::now();

        for (int i = 0; true; i++) {
            Connection connection;
            file.read(reinterpret_cast<char*>(&connection), sizeof(Connection));
            if (file.eof()) {
                break;
            }

            // Find the node in the adjacency list
            auto iter = nodeIndex.find(connection.putID);

            // Create a new node if the node does not exist
            if (iter == nodeIndex.end()) {
                nodeIndex[connection.putID] = adjacencyList.size();
                adjacencyList.push_back(connection);
                index[connection.putID] = set<string>{connection.getID};
            } else {
                adjacencyList[iter->second].connections.push_back(Node{connection.getID, connection.weight});
                index[connection.putID].insert(connection.getID);
            }

            // Create an inverted index
            auto inverted_iter = inverted_index.find(connection.getID);
            if (inverted_iter == inverted_index.end()) {
                inverted_index[connection.getID] = set<string>{connection.putID};
            } else {
                inverted_iter->second.insert(connection.putID);
            }
        }

        // Sort the adjacency list
        sort(adjacencyList.begin(), adjacencyList.end(), [](const AdjacencyListNode& a, const AdjacencyListNode& b) {
            if (a.putID != b.putID) {
                return a.putID < b.putID;
            } else {
                return a.connections.size() < b.connections.size();
            }
        });

        nodeIndex.clear();
        // Sort the connections
        int i = 0;
        for (auto& node : adjacencyList) {
            nodeIndex[node.putID] = i++;
            node.sort_connections();
            set_intersection(index[node.putID].begin(), index[node.putID].end(), inverted_index[node.putID].begin(), inverted_index[node.putID].end(), inserter(dual_side_index[node.putID], dual_side_index[node.putID].begin()));
        }

        auto end = chrono::high_resolution_clock::now();
        cout << "Elapsed time: " << chrono::duration_cast<chrono::milliseconds>(end - start).count() << " ms" << endl;
    }

    void print(string filename) {
        fstream file(filename, ios::out);

        file << "<<< There are " << adjacencyList.size() << " IDs in total. >>>" << endl;
        cout << "\n<<< There are " << adjacencyList.size() << " IDs in total. >>>" << endl;
        int i = 0;
        int total = 0;
        for (const auto& node : adjacencyList) {
            file << "[" << setw(3) << ++i << "] " << node.putID << ": " << endl;
            int j = 0;
            for (const auto& connection : node.connections) {
                total++;
                file << "\t(" << setw(2) << ++j << ") " << connection.getID << "," << setw(7) << connection.weight;
                if (j % 12 == 0) {
                    file << endl;
                }
            }
            file << endl;
        }

        cout << "\n<<< There are " << total << " nodes in total. >>>" << endl;
        file << "<<< There are " << total << " nodes in total. >>>" << endl;
    }

    void printBFS(string filename) {
        fstream file(filename, ios::out);

        file << "<<< There are " << BFS_result.size() << " IDs in total. >>>" << endl;
        int i = 0;
        for (const auto& node : BFS_result) {
            file << "[" << setw(3) << ++i << "] " << node.putID << "(" << node.connections.size() << "): " << endl;
            int j = 0;
            for (const auto& connection : node.connections) {
                file << "\t(" << setw(2) << ++j << ") " << connection;
                if (j % 12 == 0) {
                    file << endl;
                }
            }
            file << endl;
        }
    }

    void BFS() {
        BFS_result.clear();
        BFS_index.clear();

        auto start = chrono::high_resolution_clock::now();

        for (const auto& node : adjacencyList) {
            queue<string> q;
            unordered_set<string> visited;
            bool skip = false;
            auto& dual_side = dual_side_index.find(node.putID)->second;
            for (const auto& connection : dual_side) {
                auto bfs_iter = BFS_index.find(connection);
                if (bfs_iter != BFS_index.end()) {
                    BFS_index[node.putID] = BFS_result.size();
                    BFS_result.push_back(BFS_result[bfs_iter->second]);
                    BFS_result.back().putID = node.putID;
                    BFS_result.back().connections.insert(bfs_iter->first);
                    // remove the node itself
                    BFS_result.back().connections.erase(node.putID);

                    skip = true;
                    break;
                }
            }

            if (skip) {
                continue;
            }

            q.push(node.putID);
            visited.insert(node.putID);
            while (!q.empty()) {
                string current = q.front();
                q.pop();

                // if already visited, skip
                if (visited.find(current) == visited.end()) {
                    continue;
                }

                auto iter = BFS_index.find(current);
                if (iter != BFS_index.end()) {
                    for (auto connection : BFS_result[iter->second].connections) {
                        visited.insert(connection);
                    }
                } else {
                    for (const auto& connection : adjacencyList[nodeIndex[current]].connections) {
                        if (visited.find(connection.getID) == visited.end()) {
                            q.push(connection.getID);
                            visited.insert(connection.getID);
                        }
                    }
                }
            }

            BFS_index[node.putID] = BFS_result.size();

            // remove the node itself
            visited.erase(node.putID);

            BFSNode BFS_node;
            BFS_node.putID = node.putID;
            BFS_node.connections = move(set<std::string>(visited.begin(), visited.end()));
            BFS_result.push_back(std::move(BFS_node));
        }

        stable_sort(BFS_result.begin(), BFS_result.end(), [](const BFSNode& a, const BFSNode& b) {
            return a.connections.size() > b.connections.size();
        });

        auto end = chrono::high_resolution_clock::now();
        cout << "Elapsed time: " << chrono::duration_cast<chrono::milliseconds>(end - start).count() << " ms" << endl;
    }
};

int main() {
    AdjacencyList adjacencyList;
    string file_number;
    do {
        cout << "\n**** Graph data manipulation *****" << endl;
        cout << "* 0. QUIT                        *" << endl;
        cout << "* 1. Build adjacency lists       *" << endl;
        cout << "* 2. Compute connection counts   *" << endl;
        cout << "**********************************" << endl;
        cout << "Input a choice(0, 1, 2): ";
        int choice;
        cin >> choice;
        if (choice == 0) {
            break;
        } else if (choice == 1) {
            adjacencyList.clear();

            cout << "Input a file number ([0] Quit): ";
            cin >> file_number;
            fstream file("pairs" + file_number + ".bin", ios::in | ios::binary);
            if (!file) {
                cout << "\n### pairs" + file_number + ".bin" << " does not exist! ###" << endl;
                continue;
            }

            adjacencyList.build(file);
            adjacencyList.print("pairs" + file_number + ".adj");

        } else if (choice == 2) {
            adjacencyList.BFS();
            adjacencyList.printBFS("pairs" + file_number + ".cnt");
        } else {
            cout << "Command does not exist!" << endl;
        }
    } while (true);
}