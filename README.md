# openglGAME
labyrinth game implemented with opengl

![](OpenGL%20Labyrinth%20.png)

OpenGL is an API for rendering 2D and 3D vector graphics. 
The game is a labyrinth game player's point of view is the camera. 
The environment consists of 3d cubes that together form a maze. There is a key for the escape door in the labyrinth. 
There is a monster that follows the player during the game. The game ends if it catches the player. 
Its movements are slower than the player for the sake of the game. Then after the player gets the key, she gets one jump option to jump and see the labyrinth and find the door.  
The player can also escape from the monster with the jump option. The player wins if she can manage to escape the monster and find the door and open it with the key. 
The implementation consists of shaders written in GLSL. The texture of the monster and the ground are applied using mipmaps. 
The light source of the environment rotates circularly above the labyrinth. 
The shadows and Phong lightning model calculated according to the coordinates of the light source. 
Collision detection is applied to prevent the player go into the walls of the labyrinth.
