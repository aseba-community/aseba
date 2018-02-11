# A 6x6 maze playground

The 6x6 maze in this directory is designed to help test navigation
strategies. The walls can be avoided using the proximity sensors. The
large cul-de-sacs should permit a robot to escape using simple wall
avoidance. The tracks on the ground can be followed with ground
sensors. The loops in the wide green track can be followed to turn
around. The arrows on the narrow track should help the robot find the
track if it gets lost. The colored tracks are dark enough to be seen
by the proximity sensors with values around 160–180, but not so dark
that they will trigger table edge detection.

Success is when a robot finds the blue center region and stops on the
black band. Additionally this will open the blue door and allow the
second robot to escape, to test a different strategy or simply for a
victory dance.

One simple success story is to run the standard Investigator behavior
(cyan) on the robot in the top right corner, target _tcp:;port=33334_,
and run the standard Explorer behavior (yellow) on the bottommost
robot, target _tcp:;port=33333_. The explorer will succeed and stop on
the black band, opening the door for the investigator, who will run
victory laps around the green track.

The playground file was generated from the SVG file in [davidjsherman/inirobot\
-scratch-thymioII](https://github.com/davidjsherman/inirobot-scratch-thymioII/tree/master/playground/maze-6x6).
