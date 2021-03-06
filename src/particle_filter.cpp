/**
 * particle_filter.cpp
 *
 * Created on: Dec 12, 2016
 * Author: Tiffany Huang
 */

#include "particle_filter.h"

#include <math.h>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "helper_functions.h"

using std::string;
using std::vector;
using std::normal_distribution;
using std::default_random_engine;

void ParticleFilter::init(double x, double y, double theta, double std[]) {
  /**
   * TODO: Set the number of particles. Initialize all particles to 
   *   first position (based on estimates of x, y, theta and their uncertainties
   *   from GPS) and all weights to 1. 
   * TODO: Add random Gaussian noise to each particle.
   * NOTE: Consult particle_filter.h for more information about this method 
   *   (and others in this file).
   */

  default_random_engine gen;

  num_particles = 100;  

  // Set Dimensions for weights

  weights.resize(num_particles);

  // Add Gaussian noise to particles

  normal_distribution<double> dist_x(x, std[0]);
  normal_distribution<double> dist_y(y, std[1]);
  normal_distribution<double> dist_theta(theta, std[2]);

  Particle particle;

  for (int i = 0; i < num_particles; i++) {

    particle.x = dist_x(gen);
    particle.y = dist_x(gen);
    particle.theta = dist_theta(gen);
    particle.weight = 1.;

    particles.push_back(particle);
  }

  // Initialization complete
  is_initialized = true;
}

void ParticleFilter::prediction(double delta_t, double std_pos[], 
                                double velocity, double yaw_rate) {
  /**
   * TODO: Add measurements to each particle and add random Gaussian noise.
   * NOTE: When adding noise you may find std::normal_distribution 
   *   and std::default_random_engine useful.
   *  http://en.cppreference.com/w/cpp/numeric/random/normal_distribution
   *  http://www.cplusplus.com/reference/random/default_random_engine/
   */

  default_random_engine gen;

  for (int i = 0; i < num_particles; i++) {

    double x = particles[i].x;
    double y = particles[i].y;
    double theta = particles[i].theta;

    // Predict new position based on velocity, delta_t and yaw_rate

    if (yaw_rate != 0) {
      x += velocity / yaw_rate * (sin(theta + yaw_rate * delta_t) - sin(theta));
      y += velocity / yaw_rate * (cos(theta) - cos(theta + yaw_rate * delta_t));
      theta += yaw_rate * delta_t;
    }
    else {
      
      x += velocity * delta_t * cos(theta);
      y += velocity * delta_t * sin(theta); 
    }

  // Add Gaussian noise to particles

  normal_distribution<double> dist_x(x, std_pos[0]);
  normal_distribution<double> dist_y(y, std_pos[1]);
  normal_distribution<double> dist_theta(theta, std_pos[2]);

  particles[i].x = dist_x(gen);
  particles[i].y = dist_y(gen);
  particles[i].theta = dist_theta(gen);
  }

}

void ParticleFilter::dataAssociation(vector<LandmarkObs> predicted, 
                                     vector<LandmarkObs>& observations) {
  /**
   * TODO: Find the predicted measurement that is closest to each 
   *   observed measurement and assign the observed measurement to this 
   *   particular landmark.
   * NOTE: this method will NOT be called by the grading code. But you will 
   *   probably find it useful to implement this method and use it as a helper 
   *   during the updateWeights phase.
   */

}

void ParticleFilter::updateWeights(double sensor_range, double std_landmark[], 
                                   const vector<LandmarkObs> &observations, 
                                   const Map &map_landmarks) {
  /**
   * TODO: Update the weights of each particle using a mult-variate Gaussian 
   *   distribution. You can read more about this distribution here: 
   *   https://en.wikipedia.org/wiki/Multivariate_normal_distribution
   * NOTE: The observations are given in the VEHICLE'S coordinate system. 
   *   Your particles are located according to the MAP'S coordinate system. 
   *   You will need to transform between the two systems. Keep in mind that
   *   this transformation requires both rotation AND translation (but no scaling).
   *   The following is a good resource for the theory:
   *   https://www.willamette.edu/~gorr/classes/GeneralGraphics/Transforms/transforms2d.htm
   *   and the following is a good resource for the actual equation to implement
   *   (look at equation 3.33) http://planning.cs.uiuc.edu/node99.html
   */


  // Some variables for brevity

  double var_x = std_landmark[0] * std_landmark[0];
  double var_y = std_landmark[1] * std_landmark[1];
  double norm_term = 1 / ( 2 * M_PI * std_landmark[0] * std_landmark[1]);
  
  // Variables created outside loop for speed

  Particle particle;
  double landmark_dist, exponent, weight, min_dist;
  Map::single_landmark_s landmark;
  LandmarkObs t_observation;
  int index;

  // For each particle, transform to map co-ord, find closest landmark and assign weight

  for (int i = 0; i < num_particles; i++) {

    particle = particles[i];
    particle.weight = 1.;    

    vector<int> associations;
    vector<double> sense_x;
    vector<double> sense_y;

    // Transform to map co-ord and find closest landmark

    for (int j = 0; j < observations.size(); j++) {

      t_observation.id = observations[j].id;
      t_observation.x = particle.x + cos(particle.theta) * observations[j].x
                      - sin(particle.theta) * observations[j].y;
      t_observation.y = particle.y + sin(particle.theta) * observations[j].x
                      + cos(particle.theta) * observations[j].y;

      index = 0;

      min_dist = sensor_range;

      for (int k = 0; k < map_landmarks.landmark_list.size(); k++) {

        landmark = map_landmarks.landmark_list[k];
        landmark_dist = dist(t_observation.x, t_observation.y, landmark.x_f, landmark.y_f);

        if (landmark_dist < min_dist) {

          min_dist = landmark_dist;
          index = k;
        }
      }

      // Update weight

      landmark = map_landmarks.landmark_list[index];

      exponent = (pow(t_observation.x - landmark.x_f, 2) / (2. * var_x))
                  + (pow(t_observation.y - landmark.y_f, 2) / (2. * var_y));

      weight = norm_term * exp(-exponent);
      particle.weight *= weight;

      associations.push_back(index + 1);
      sense_x.push_back(t_observation.x);
      sense_y.push_back(t_observation.y);

    }


    particles[i].weight = particle.weight;
    weights[i] = particle.weight;

    SetAssociations(particles[i], associations, sense_x, sense_y);

  }

}

void ParticleFilter::resample() {
  /**
   * TODO: Resample particles with replacement with probability proportional 
   *   to their weight. 
   * NOTE: You may find std::discrete_distribution helpful here.
   *   http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution
   */

  // Placeholder for resampled particles

  vector<Particle> resampled_particles;

  default_random_engine gen;

  std::discrete_distribution<int> index_dist(weights.begin(), weights.end());

  int current_index;

  for (unsigned int i = 0; i < particles.size(); i++) {

    current_index = index_dist(gen);

    resampled_particles.push_back(particles[current_index]);
  }

  particles = resampled_particles;

}

void ParticleFilter::SetAssociations(Particle& particle, 
                                     const vector<int>& associations, 
                                     const vector<double>& sense_x, 
                                     const vector<double>& sense_y) {
  // particle: the particle to which assign each listed association, 
  //   and association's (x,y) world coordinates mapping
  // associations: The landmark id that goes along with each listed association
  // sense_x: the associations x mapping already converted to world coordinates
  // sense_y: the associations y mapping already converted to world coordinates
  particle.associations= associations;
  particle.sense_x = sense_x;
  particle.sense_y = sense_y;
}

string ParticleFilter::getAssociations(Particle best) {
  vector<int> v = best.associations;
  std::stringstream ss;
  copy(v.begin(), v.end(), std::ostream_iterator<int>(ss, " "));
  string s = ss.str();
  s = s.substr(0, s.length()-1);  // get rid of the trailing space
  return s;
}

string ParticleFilter::getSenseCoord(Particle best, string coord) {
  vector<double> v;

  if (coord == "X") {
    v = best.sense_x;
  } else {
    v = best.sense_y;
  }

  std::stringstream ss;
  copy(v.begin(), v.end(), std::ostream_iterator<float>(ss, " "));
  string s = ss.str();
  s = s.substr(0, s.length()-1);  // get rid of the trailing space
  return s;
}