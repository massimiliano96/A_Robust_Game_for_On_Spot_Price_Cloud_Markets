#ifndef _GAME_H_
#define _GAME_H_

#include "Set_System.hpp"
#include "SaaS_Problem.hpp"
#include "SaaS.hpp"
#include "IaaS.hpp"
#include "IaaS_Problem.hpp"
#include "Set_System.hpp"
#include <string>
#include <memory>
#include <vector>
#include <iostream>
#include <fstream>


class Game
{
private:

  std::vector< std::shared_ptr<SaaS_Problem> > SaaSs;
  std::shared_ptr< IaaS_Problem > iaas ;
  unsigned n_app = 0;
  double iaas_time = .0;

  // private_methods
  void create_global_file( std::fstream& );
  void create_SaaSs_file( std::fstream& );
  void create_WSs_file( std::fstream& );

public:

  // constructor
  Game() = default;
  Game( unsigned seed, const std::string&, const std::string&, const std::string&, const std::string& );
  Game( unsigned seed, const std::string& );


  //getters
  std::vector<std::shared_ptr<SaaS_Problem>> get_SaaSs( void );

  std::shared_ptr< IaaS_Problem > get_iaas( void );

  //methods
  double compute_total_n( void );

  bool check_system( void );

  std::pair< bool, std::vector<bool> > check_changes( void );

  void create_files( void ); // this function create the files of the parameters

  void solve( void ); // this function soleve the problem

  void print( std::ofstream& );

  void reset_SaaSs_given_on_spot( void );     // this function reset the given_on_spot for each SaaS before of each call of solve_greedy()

};
#endif
