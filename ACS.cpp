#include "ACS.h"
#include <algorithm>
ACS::ACS(double a, double b, double e, int colonySize, int numIterations,
         vector<vector<double>> cityDists, double t0, double wearFactor, double q0):
AntSystem(a, b, e, colonySize, numIterations, cityDists, t0) {
    this->epsilon = wearFactor;
    this->q_0 = q0;
    this->tau_0 = 1/(colonySize * length_nn()); //input t0 in unecessary
    init_phers();
    //cout << lookup_pher(4, 10) << endl;
    //cout << tau_0 << endl;
    srand(time(NULL));
}

void ACS::init_phers() {
  for(int i = 0; i < num_cities; ++i)
        for(int j = 0; j < i; ++j)
            pheromones[i][j] = tau_0;
}

Result ACS::runACS() {
  double start_time = clock();
  double curr_best = MAX_DOUBLE;

  clear_ants(); // also initiallizes
  for(int i = 0; i < num_iterations; ++i) {
    make_tours();
    wear_away();
    add_pheromone();

    clear_ants();
    if(i % 10 == 0) {
      results.best_ant_every_10.push_back(best_ant.length);
      //      cout << "Iteration " << i << ": " << best_ant.length;
      //      cout << endl;
    }
    if(best_ant.length < curr_best) {
      curr_best = best_ant.length;
      results.iteration_of_best_ant = i;
      for(int j = 0; j < best_ant.tour.size(); ++j) {
	// cout << best_ant.tour[j] << ", ";
      }
      //cout << endl;
    }
  }

  double end_time = clock();

  results.greedy_result = length_nn();
  results.best_length = best_ant.length;
  results.run_time = (end_time - start_time)/CLOCKS_PER_SEC;

  //  cout << "The shortest ACS path is " << best_ant.length << endl;
  //  cout << "The shortest greedy path is " << results.greedy_result << endl;
  //  cout << "Runtime: " << (end_time - start_time)/CLOCKS_PER_SEC << endl;

  return results;
}


// add pheromone to every ant path on bsf and evaporate from all
void ACS::add_pheromone() {

    //evaporate pheromone
    for(int i = 1; i < num_cities; ++i) {
        for(int j = 0; j < i; ++j) {
            pheromones[i][j] = (1 - evap_rate) * pheromones[i][j];
        }
    }

    // add pheromone to paths on bsf
    for(int j = 0; j < num_cities; ++j) {
        int start_city = best_ant.tour[j];
        int end_city;
        if(j < num_cities - 1) {
            end_city = best_ant.tour[j+1];
        }
        else {
            end_city = best_ant.tour[0];
        }
        if(start_city > end_city) {
            pheromones[start_city][end_city] = evap_rate / lookup_dist(start_city, end_city);
        }
        else {
            pheromones[end_city][start_city] = evap_rate / lookup_dist(start_city, end_city);
        }
    }
}


// wear away the paths of all the ants
void ACS::wear_away() {
    for(int i = 0; i < colony_size; ++i) {
        //THIS WILL SEGFAULT UNLESS THE ANTS LAST & FIRST START IS INLCUDED
        for(int j = 0; j < num_cities; ++j) {
            //cout << j << endl;
            int start_city = colony[i].tour[j];
            int end_city;
            if(j < num_cities - 1) {
                end_city = colony[i].tour[j+1];
            }
            else {
                end_city = colony[i].tour[0];
            }
            if(start_city > end_city) {
                pheromones[start_city][end_city] = (1 - epsilon) * lookup_pher(start_city, end_city) + tau_0 * epsilon;
            }
            else {
                pheromones[end_city][start_city] = (1 - epsilon) * lookup_pher(start_city, end_city) + tau_0 * epsilon;
            }
        }
    }
}


// take the exploitation step that maximizes pheromone/distance
void ACS::exploitation_step(int ant_index) {

    int max_city;
    double max_city_value = -1;
    int curr_city = colony[ant_index].tour.back();

    for(int i = 0; i < num_cities; ++i) {
        if(colony[ant_index].unvisited[i] == true) {
            double path_dist = lookup_dist(curr_city, i);
            double path_pher = lookup_pher(curr_city, i);


            double city_value = path_pher * pow(1 / path_dist, beta);
            //cout << city_value << endl;
            // if this city is the new best, update
            if(max_city_value < city_value) {
                max_city = i;
                max_city_value = city_value;
            }
        }
    }

    // update the ant's tour and unvisited vector
    //cout << max_city << endl;
    colony[ant_index].unvisited[max_city] = false;
    //cout << "unvisited" << endl;
    colony[ant_index].tour.push_back(max_city);
    colony[ant_index].length += lookup_dist(curr_city, max_city);
    //cout << "Saved" << endl;
}

void ACS::make_tours() {
  for(int i = 0; i < colony_size; ++i) {


    int starting_city = rand() % num_cities;

    colony[i].tour.push_back(starting_city);
    colony[i].unvisited[starting_city] = false;

    for(int j = 0; j < num_cities - 1; ++j) {
      double step_prob = (double) rand() / RAND_MAX;

      if(step_prob < q_0) {
	//cout << "Exploitation" << endl;
	exploitation_step(i);

      }
      else {
	//cout << "Probabilistic" << endl;
	probabilistic_next_step(i);
      }

    }

    //cout << "mostly done tours" << endl;
    // make ant return to starting city


    //cout << colony[i].tour.back() << endl;
    colony[i].length += lookup_dist(starting_city, colony[i].tour.back());
    colony[i].tour.push_back(starting_city);
    // update best ant as necessary
    if(colony[i].length < best_ant.length) {
      best_ant = colony[i];
    }
  }
}
