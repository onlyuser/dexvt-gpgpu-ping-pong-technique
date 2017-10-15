[![Build Status](https://secure.travis-ci.org/onlyuser/dexvt-gpgpu-ping-pong-technique.png)](http://travis-ci.org/onlyuser/dexvt-gpgpu-ping-pong-technique)

dexvt-gpgpu-ping-pong-technique
===============================

Copyright (C) 2011-2017 <mailto:onlyuser@gmail.com>

About
-----

dexvt-gpgpu-ping-pong-technique is a GPGPU Ping-pong technique demonstration.

See sister project demonstrating shader features: [dexvt-test](https://github.com/onlyuser/dexvt-test)

Conway's Game of Life
---------------------

[![Screenshot](https://sites.google.com/site/onlyuser/projects/graphics/thumbs/dexvt-gpgpu_conway_thumb.gif?attredirects=0)](https://sites.google.com/site/onlyuser/projects/graphics/images/dexvt-gpgpu_conway.gif?attredirects=0)

1. Any live cell with fewer than two live neighbours dies
2. Any live cell with two or three live neighbours lives on
3. Any live cell with more than three live neighbours dies
4. Any dead cell with exactly three live neighbours becomes a live cell

Maze Solver
-----------

[![Screenshot](https://sites.google.com/site/onlyuser/projects/graphics/thumbs/dexvt-gpgpu_maze_thumb.gif?attredirects=0)](https://sites.google.com/site/onlyuser/projects/graphics/images/dexvt-gpgpu_maze.gif?attredirects=0)

1. Prune maze walls from endpoints
2. Grow maze walls without changing topology
3. Build distance field from every cell on grid to cursor location
4. Guide individual sprites along distance field gradient

(NOTE: steps 1 & 2 are not necessary for maze solving)

Requirements
------------

Unix tools and 3rd party components (accessible from $PATH):

    gcc mesa-common-dev freeglut3-dev libglew-dev libglm-dev libpng-dev

Make Targets
------------

<table>
    <tr><th> target     </th><th> action                        </th></tr>
    <tr><td> all        </td><td> make binaries                 </td></tr>
    <tr><td> test       </td><td> all + run tests               </td></tr>
    <tr><td> clean      </td><td> remove all intermediate files </td></tr>
    <tr><td> lint       </td><td> perform cppcheck              </td></tr>
    <tr><td> docs       </td><td> make doxygen documentation    </td></tr>
    <tr><td> clean_lint </td><td> remove cppcheck results       </td></tr>
    <tr><td> clean_docs </td><td> remove doxygen documentation  </td></tr>
</table>

Conway's Game of Life Controls
------------------------------

Keyboard:

<table>
    <tr><th> key   </th><th> purpose           </th></tr>
    <tr><td> r     </td><td> reset canvas      </td></tr>
    <tr><td> f     </td><td> toggle frame rate </td></tr>
    <tr><td> h     </td><td> toggle HUD        </td></tr>
    <tr><td> space </td><td> toggle animation  </td></tr>
    <tr><td> esc   </td><td> exit              </td></tr>
</table>

Maze Solver Controls
--------------------

Keyboard:

<table>
    <tr><th> key   </th><th> purpose                        </th></tr>
    <tr><td> r     </td><td> respawn sprites                </td></tr>
    <tr><td> f1    </td><td> regenerate maze                </td></tr>
    <tr><td> f2    </td><td> regenerate maze + prune        </td></tr>
    <tr><td> f3    </td><td> regenerate maze + prune + grow </td></tr>
    <tr><td> f     </td><td> toggle frame rate              </td></tr>
    <tr><td> h     </td><td> toggle HUD                     </td></tr>
    <tr><td> space </td><td> toggle animation               </td></tr>
    <tr><td> esc   </td><td> exit                           </td></tr>
</table>

References
----------

<dl>
    <dt>"Conway's Game of Life"</dt>
    <dd>https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life</dd>
    <dt>"Anand's blog - GPGPU In Android: Load a texture with floats and read it back"</dt>
    <dd>http://www.anandmuralidhar.com/blog/tag/gpgpu/</dd>
    <dt>"A GPU Approach to Conway's Game of Life"</dt>
    <dd>http://nullprogram.com/blog/2014/06/10/</dd>
    <dt>"A GPU Approach to Path Finding"</dt>
    <dd>http://nullprogram.com/blog/2014/06/22/#comment-1450664791</dd>
    <dt>"Understanding Goal-Based Vector Field Pathfinding"</dt>
    <dd>https://gamedevelopment.tutsplus.com/tutorials/understanding-goal-based-vector-field-pathfinding--gamedev-9007</dd>
    <dt>"realtimecollisiondetection.net – Aiding pathfinding with cellular automata"</dt>
    <dd>http://realtimecollisiondetection.net/blog/?p=57</dd>
</dl>

Keywords
--------

    GPGPU, ping-pong technique, OpenGL, glsl shader, glm, Conway's Game of Life, cellular automata
