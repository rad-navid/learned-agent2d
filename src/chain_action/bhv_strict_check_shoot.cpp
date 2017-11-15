// -*-c++-*-

#include <rcsc/action/kick_table.h>
#include <rcsc/game_time.h>
#include <rcsc/player/action_effector.h>
#include <rcsc/player/ball_object.h>
#include <rcsc/player/self_object.h>
#include <rcsc/player/world_model.h>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <vector>
#include <queue>
#include <iostream>
#include <fstream>
using namespace std;

/*!
  \file bhv_strict_check_shoot.cpp
  \brief strict checked shoot behavior using ShootGenerator
*/

/*
 *Copyright:

 Copyright (C) Hidehisa AKIYAMA

 This code is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 3 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 *EndCopyright:
 */

/////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "bhv_strict_check_shoot.h"

#include "shoot_generator.h"

#include <rcsc/action/neck_turn_to_goalie_or_scan.h>
#include <rcsc/action/neck_turn_to_point.h>
#include <rcsc/action/body_smart_kick.h>
#include <rcsc/player/player_agent.h>
#include <rcsc/player/debug_client.h>
#include <rcsc/common/logger.h>

using namespace rcsc;

/*-------------------------------------------------------------------*/
/*!

*/
bool
Bhv_StrictCheckShoot::execute( PlayerAgent * agent )
{
    const WorldModel & wm = agent->world();

    if ( ! wm.self().isKickable() )
    {
        std::cerr << __FILE__ << ": " << __LINE__
                  << " not ball kickable!"
                  << std::endl;
        dlog.addText( Logger::ACTION,
                      __FILE__":  not kickable" );
        return false;
    }

    const ShootGenerator::Container & cont = ShootGenerator::instance().courses( wm );

    // update
    if ( cont.empty() )
    {
        dlog.addText( Logger::SHOOT,
                      __FILE__": no shoot course" );
        return false;
    }

    ShootGenerator::Container::const_iterator best_shoot
        = std::min_element( cont.begin(),
                            cont.end(),
                            ShootGenerator::ScoreCmp() );

    if ( best_shoot == cont.end() )
    {
        dlog.addText( Logger::SHOOT,
                      __FILE__": no best shoot" );
        return false;
    }

    // it is necessary to evaluate shoot courses

    agent->debugClient().addMessage( "Shoot" );
    agent->debugClient().setTarget( best_shoot->target_point_ );

    Vector2D one_step_vel
        = KickTable::calc_max_velocity( ( best_shoot->target_point_ - wm.ball().pos() ).th(),
                                        wm.self().kickRate(),
                                        wm.ball().vel() );
    double one_step_speed = one_step_vel.r();

    dlog.addText( Logger::SHOOT,
                  __FILE__": shoot[%d] target=(%.2f, %.2f) first_speed=%f one_kick_max_speed=%f",
                  best_shoot->index_,
                  best_shoot->target_point_.x,
                  best_shoot->target_point_.y,
                  best_shoot->first_ball_speed_,
                  one_step_speed );

    if ( one_step_speed > best_shoot->first_ball_speed_ * 0.99 )
    {
        if ( Body_SmartKick( best_shoot->target_point_,
                             one_step_speed,
                             one_step_speed * 0.99 - 0.0001,
                             1 ).execute( agent ) )
        {

             agent->setNeckAction( new Neck_TurnToGoalieOrScan( -1 ) );
             agent->debugClient().addMessage( "Force1Step" );
             mlLog(agent,best_shoot->target_point_,one_step_speed);
             return true;
        }
    }

    if ( Body_SmartKick( best_shoot->target_point_,
                         best_shoot->first_ball_speed_,
                         best_shoot->first_ball_speed_ * 0.99,
                         3 ).execute( agent ) )
    {
        if ( ! doTurnNeckToShootPoint( agent, best_shoot->target_point_ ) )
        {
            agent->setNeckAction( new Neck_TurnToGoalieOrScan( -1 ) );
        }
        mlLog(agent,best_shoot->target_point_,best_shoot->first_ball_speed_);
        return true;
    }

    dlog.addText( Logger::SHOOT,
                  __FILE__": failed" );
    return false;
}


void Bhv_StrictCheckShoot::mlLog(rcsc::PlayerAgent * agent,
		const rcsc::Vector2D & shoot_point, const double vel) {

	/*
	 * this is the order of features
	 * time, ball.x, ball.y, target.x, target,y, ball.vel, goali_line_dist, goali_ball_dist, top 4 opp_line_dist, top 4 opp_ball_dist
	 */
	ofstream myfile;
	myfile.open("/home/navid/robo/ml-log.txt", ios::app);

	const WorldModel & wm = agent->world();
	myfile << wm.time().cycle()<<"\t"<< wm.ball().pos().x<<"\t"<< wm.ball().pos().y<<"\t" << shoot_point.x << "\t" << shoot_point.y << "\t"<< vel <<"\t";

	double opp_goali_ball_dist = 1000;
	double opp_goali_line_dist = 1000;
	std::priority_queue<std::pair<double, int> > opp_line_dist;
	std::priority_queue<std::pair<double, int> > opp_ball_dist;
	opp_line_dist.push(std::pair<double, int>(-1000, 0));
	opp_ball_dist.push(std::pair<double, int>(-1000, 0));

	Line2D shoot_line(shoot_point, wm.ball().pos());
	const PlayerPtrCont::const_iterator end = wm.opponentsFromBall().end();
	int index=1;
	for (PlayerPtrCont::const_iterator p = wm.opponentsFromBall().begin();p != end; ++p) {
		double line_dist = shoot_line.dist((*p)->pos());
		double ball_dist = wm.ball().pos().dist((*p)->pos());
		if ((*p)->goalie()) {
			opp_goali_ball_dist = ball_dist;
			opp_goali_line_dist = line_dist;
			myfile << opp_goali_line_dist<<"\t";
			myfile << opp_goali_ball_dist<<"\t";
		} else if((*p)->pos().x+10 > wm.self().pos().x ){
			opp_line_dist.push(std::pair<double, int>(-1*line_dist, index));
			opp_ball_dist.push(std::pair<double, int>(-1*ball_dist, index));
			index++;
		}

	}

	// pick top 4 distances from line
	int k = 4; // number of indices we need
	for (int i = 0; i < k; ++i) {
		double dist = opp_line_dist.top().first;
		myfile << -1*dist <<"\t";
		opp_line_dist.pop();
	}
	//pick top 4 distances from ball
	for (int i = 0; i < k; ++i) {
			double dist = opp_ball_dist.top().first;
			myfile << -1*dist <<"\t";
			opp_ball_dist.pop();
	}

	myfile << std::endl;;
	myfile.close();
}


/*-------------------------------------------------------------------*/
/*!

 */
bool
Bhv_StrictCheckShoot::doTurnNeckToShootPoint( PlayerAgent * agent,
                                              const Vector2D & shoot_point )
{
    const double angle_buf = 10.0; // Magic Number

    if ( ! agent->effector().queuedNextCanSeeWithTurnNeck( shoot_point, angle_buf ) )
    {
        dlog.addText( Logger::TEAM,
                      __FILE__": cannot look the shoot point(%.2f %.2f)",
                      shoot_point.x,
                      shoot_point.y );
        return false;
    }

#if 0
    const WorldModel & wm = agent->world();
    if ( wm.seeTime() == wm.time() )
    {
        double current_width = wm.self().viewWidth().width();
        AngleDeg target_angle = ( shoot_point - wm.self().pos() ).th();
        double angle_diff = ( target_angle - wm.self().face() ).abs();

        if ( angle_diff < current_width*0.5 - angle_buf )
        {
            dlog.addText( Logger::TEAM,
                          __FILE__": already seen. width=%.1f, diff=%.1f. shoot point(%.2f %.2f)",
                          current_width,
                          angle_diff,
                          shoot_point.x,
                          shoot_point.y );
            return false;
        }
    }
#endif

    dlog.addText( Logger::TEAM,
                  __FILE__": turn_neck to the shoot point(%.2f %.2f)",
                  shoot_point.x,
                  shoot_point.y );
    agent->debugClient().addMessage( "Shoot:NeckToTarget" );

    agent->setNeckAction( new Neck_TurnToPoint( shoot_point ) );

    return true;
}
