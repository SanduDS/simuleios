/*-------------huffman_vis.cpp------------------------------------------------//
*
* Purpose: To visualize a simple huffman tree for LeiosOS
*
*   Notes: This will be using the cairo package, hopefully creating animations
*          I could use the subroutine-comic project, but this will be from 
*          scratch
*
*-----------------------------------------------------------------------------*/

#include <cairo.h>
#include <iostream>
#include <iomanip>
#include <math.h>
#include <string>
#include <vector>
#include <sstream>
#include <random>
#include "huffman.h"

//#define num_frames 300
#define num_frames 200

// Struct to hold positions
struct pos{
    double x, y;
};

// Struct for colors
struct color{
    double r, g, b;
};

// Struct to hold all the necessary data for animations
struct frame{
    int res_x, res_y;
    int fps;
    int curr_frame;
    cairo_surface_t *frame_surface[num_frames];
    cairo_t *frame_ctx[num_frames];
    cairo_surface_t *bg_surface;
    cairo_t *bg_ctx;
    pos origin;
    std::string pngbase;

    // Function to call frame struct
    void create_frame(int x, int y, int ps, std::string pngname);

    // Function to initialize the frame struct
    void init();

    // Function to draw all frames in the frame struct
    void draw_frames();

    // Function to destroy all contexts and surfaces
    void destroy_all();

};

// Function to create basic colored background
void create_bg(frame &anim, int r, int g, int b);

// Function to grow a circle at a provided point
void grow_circle(frame &anim, double time, pos ori, double radius);

// Function to animate a line from two points
void animate_line(frame &anim, int start_frame, double time, 
                  pos ori_1, double radius_1, pos ori_2, double radius_2);

// Function to draw huffman tree
void draw_external(frame &anim, double time, huffman_tree tree);

// finding x pos for huffman tree drawing
double find_x_pos(frame &anim, std::string bitstring);

// Function to draw internal_nodes
void draw_internal(frame &anim, double time, node_queue regenerated_nodes, 
                   double radius, huffman_tree final_tree);

// Overloaded create_nodes function for root node
// This time we are recreating the tree for later
node_queue regenerate_nodes(frame &anim, node *root, 
                            std::unordered_map<char, std::string> bitmap);

// Does a simple search to regenerated node_queue from root
void depth_first_search(frame &anim, node* root, node_queue &regenerated_nodes,
                        std::unordered_map<char, std::string> bitmap);

// Function to draw layers
void draw_layers(std::vector<frame> layer);

/*----------------------------------------------------------------------------//
* MAIN
*-----------------------------------------------------------------------------*/

int main(){

    std::vector<frame> layer(3);
    for (size_t i = 0; i < layer.size(); ++i){
        layer[i].create_frame(400, 300, 10, "frames/image");
        layer[i].init();

        layer[i].curr_frame = 1;
    }

    create_bg(layer[0], 0, 0, 0);

    // encoding with 2-pass huffman
    huffman_tree final_tree = two_pass_huffman("Jack and Jill went up the hill to fetch a pail of water. Jack fell down and broke his crown and Jill came Tumbling after! \nWoo!");
    decode(final_tree);

    std::cout << "final_tree root weight is: " 
              << final_tree.root->weight << '\n';

    node_queue regenerated_nodes = regenerate_nodes(layer[0],final_tree.root,
                                                    final_tree.bitmap);

    draw_external(layer[1], 10.0, final_tree);
    layer[2].curr_frame = layer[1].curr_frame;
    draw_internal(layer[2], 10.0, regenerated_nodes, 10, final_tree);

    draw_layers(layer);

} 

// Function to initialize the frame struct
void frame::init(){
    int line_width = 3;
    for (size_t i = 0; i < num_frames; ++i){
        frame_surface[i] = 
            cairo_image_surface_create(CAIRO_FORMAT_ARGB32, res_x, res_y);
        frame_ctx[i] = cairo_create(frame_surface[i]);
        cairo_set_line_cap(frame_ctx[i], CAIRO_LINE_CAP_ROUND);
        cairo_set_line_width(frame_ctx[i], line_width);
        cairo_set_font_size(frame_ctx[i], 20.0);
    }
    bg_surface = 
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, res_x, res_y);
    bg_ctx = cairo_create(bg_surface);
    curr_frame = 0;
}

// Creating basic colored background
void create_bg(frame &anim, int r, int g, int b){
    for (int i = 0; i < num_frames; ++i){
        cairo_set_source_rgb(anim.frame_ctx[i],(double)r, (double)g, (double)b);
        cairo_rectangle(anim.frame_ctx[i],0,0,anim.res_x,anim.res_y);
        cairo_fill(anim.frame_ctx[i]);
    }
}

// Function to draw all frames in the frame struct
void frame::draw_frames(){
    std::string pngid, number;
    for (size_t i = 0; i < num_frames; ++i){
        cairo_set_source_surface(frame_ctx[i], frame_surface[i], 0, 0);
        cairo_paint(frame_ctx[i]);

        // Setting up number with stringstream
        std::stringstream ss;
        ss << std::setw(5) << std::setfill('0') << i;
        number = ss.str();

        pngid = pngbase + number + ".png";
        std::cout << pngid << '\n';
        cairo_surface_write_to_png(frame_surface[i], pngid.c_str());
    }

}

// Function to set the initial variables
void frame::create_frame(int x, int y, int ps, std::string pngname){
    res_x = x;
    res_y = y;
    pngbase = pngname;
    fps = ps;
    origin.x = (double)x / 2.0;
    origin.y = (double)y / 2.0;
}

// Function to grow a circle at a provided point
void grow_circle(frame &anim, double time, pos ori, double radius){

    // Number of frames 
    int draw_frames = time * anim.fps;

    double curr_radius = 0;

    // internal counts that definitely start at 0
    int j = 0, k = 0;

    for (int i = anim.curr_frame; i < num_frames; ++i){
        if (i < anim.curr_frame + draw_frames){
            //expansion step
            if (i < anim.curr_frame + ceil(draw_frames * 0.5)){
                j++;
                curr_radius = (double)j * (radius * 1.25) 
                              / (double)ceil(draw_frames * 0.5);
                //std::cout << "j is: " << j << '\t' << "curr_radius is: "
                //          << curr_radius << '\n';

            }
            // Relaxation step
            else{
                k++;
                curr_radius = (radius * 1.25) + radius*((double)k * (1.0 - 1.25)
                              / (double)ceil(draw_frames * 0.5));
                //std::cout << "k is: " << k << '\t' << "curr_radius is: "
                //          << curr_radius << '\n';
            }
            cairo_arc(anim.frame_ctx[i], ori.x, ori.y, 
                      curr_radius, 0, 2*M_PI);

        }
        else{
            cairo_arc(anim.frame_ctx[i], ori.x, ori.y, 
                      radius, 0, 2*M_PI);
        }

        cairo_set_source_rgb(anim.frame_ctx[i], .25, 1, .25);

        cairo_fill(anim.frame_ctx[i]);

        cairo_stroke(anim.frame_ctx[i]);

        
    }

    //std::cout << "finished loop" << '\n';
    anim.curr_frame += draw_frames;
    std::cout << anim.curr_frame << '\n';
}

// Function to animate a line from two points
void animate_line(frame &anim, int start_frame, double time,  
                  pos ori_1, double radius_1, pos ori_2, double radius_2){

    // Finding number of frames
    int draw_frames = time * anim.fps;

    // internal count that definitely starts at 0;
    int j = 0;

    if (ori_1.x > ori_2.x){
        ori_1.x -= radius_1;
        ori_2.x += radius_2;
    }
    else{
        ori_1.x += radius_1;
        ori_2.x -= radius_2;
    }
    if (ori_1.y > ori_2.y){
        ori_1.y -= radius_1;
        ori_2.y += radius_2;
    }
    else{
        ori_1.y += radius_1;
        ori_2.y -= radius_2;
    }

    double curr_x, curr_y;

    for (int i = start_frame; i < num_frames; ++i){
        cairo_move_to(anim.frame_ctx[i], ori_1.x, ori_1.y);
        if (i < start_frame + draw_frames){
            j++;
            curr_x = ori_1.x + (double)j * (ori_2.x - ori_1.x)
                               / (double)draw_frames;
            curr_y = ori_1.y + (double)j * (ori_2.y - ori_1.y)
                               / (double)draw_frames;
            cairo_line_to(anim.frame_ctx[i], curr_x, curr_y);
        }
        else{
            cairo_line_to(anim.frame_ctx[i], ori_2.x, ori_2.y);
        }

        cairo_set_source_rgb(anim.frame_ctx[i], 1, 1, 1);
        cairo_stroke(anim.frame_ctx[i]);

    }

    if (start_frame + draw_frames > anim.curr_frame){
        anim.curr_frame = draw_frames + start_frame;
    }

}

// Function to draw huffman tree
void draw_external(frame &anim, double time, huffman_tree tree){

    // Creating positions for external nodes
    std::vector<pos> external_nodes;
    std::vector<std::string> characters;

    double lowest_location = log10(1.0 / tree.root->weight);

    pos temp_pos;

    // Determining x and y positions
    // NOTE: The .9 and .05 are arbitrary and should be modified in final form
    for (auto key : tree.bitmap){
        temp_pos.y = (log10(tree.weightmap[key.first] / tree.root->weight) 
                      / lowest_location) 
                     * .9 * anim.res_y + anim.res_y * 0.05;
        temp_pos.x = find_x_pos(anim, key.second);
        external_nodes.push_back(temp_pos);
        characters.push_back({key.first});

        std::cout << key.first << '\t' << temp_pos.x << '\t'
                  << temp_pos.y << '\n';
    }

    std::cout << tree.bitmap.size() << '\t' << external_nodes.size() << '\n';

    for (size_t i = 0; i < external_nodes.size(); ++i){
        grow_circle(anim, time / (double)external_nodes.size(), 
                    external_nodes[i], 10);

        //temp_char[0] = characters[i];

        // Placing text in circle
        for (int j = anim.curr_frame; j < num_frames; ++j){
            cairo_set_source_rgb(anim.frame_ctx[j], 0, 0, 0);
            cairo_text_extents_t textbox;
            cairo_text_extents(anim.frame_ctx[j], 
                               characters[i].c_str(),
                               &textbox);
            cairo_move_to(anim.frame_ctx[j], 
                          external_nodes[i].x - textbox.width / 2.0,
                          external_nodes[i].y + textbox.height / 2.0);
            cairo_show_text(anim.frame_ctx[j], characters[i].c_str());
            cairo_stroke(anim.frame_ctx[j]);
        }
        
    }

}

// finding x pos for huffman tree drawing
double find_x_pos(frame &anim, std::string bitstring){

    double x_pos = anim.res_x / 2.0;
    for (size_t i = 0; i < bitstring.size(); ++i){
        if (bitstring[i] == '0'){
            //std::cout << "found a 0" << '\n';
            x_pos += anim.res_x / pow(2,(double(i) + 2.0));
        }
       if (bitstring[i] == '1'){
           //std::cout << "Found a 1" << '\n';
           x_pos -= anim.res_x / pow(2,(double(i) + 2.0));
       }

       x_pos = ((x_pos - 0.5 * anim.res_x) * .95) + 0.5 * anim.res_x;
    }

    return x_pos;
}

// Function to draw internal_nodes
void draw_internal(frame &anim, double time, node_queue regenerated_nodes, 
                   double radius, huffman_tree final_tree){

    pos ori, ori_2;
    double lowest_location = log10(1.0 / final_tree.root->weight);
    double angle;
    int num_lines = 30;

    node *node1, *node2, *node_parent;
    while (regenerated_nodes.size() > 1){
        node1 = regenerated_nodes.top();
        regenerated_nodes.pop();
        node2 = regenerated_nodes.top();
        regenerated_nodes.pop();

        node_parent = new node();

        node_parent->weight = node1->weight + node2->weight;
        node_parent->x = (node1->x + node2->x) * 0.5;
        node_parent->y = (log10(node_parent->weight /final_tree.root->weight)
                          / lowest_location) 
                         * .9 * anim.res_y + anim.res_y * 0.05;        
        node1->y = (log10(node1->weight / final_tree.root->weight) 
                    / lowest_location) 
                    * .9 * anim.res_y + anim.res_y * 0.05;  
        node2->y = (log10(node2->weight / final_tree.root->weight) 
                    /lowest_location)
                    * .9 * anim.res_y + anim.res_y * 0.05; 
        node_parent->left = node1;
        node_parent->right = node2;

        // visualize this new node with lines...
        // Drawing first line
        ori.x = node1->x; ori.y = node1->y;
        ori_2.x = node_parent->x; ori_2.y = node_parent->y;
        if (node1->key){
            angle = atan2((ori_2.y - ori.y), (ori_2.x - ori.x));
            ori.x = ori.x + radius * cos(angle);
            ori.y = ori.y + radius * sin(angle);
        }
        animate_line(anim,anim.curr_frame,time/num_lines,ori,0,ori_2,0);

        ori.x = node2->x; ori.y = node2->y;
        if (node2->key){
            angle = atan2((ori_2.y - ori.y), (ori_2.x - ori.x));
            ori.x = ori.x + radius * cos(angle);
            ori.y = ori.y + radius * sin(angle);
        }
        animate_line(anim,anim.curr_frame-((time/num_lines) * anim.fps),
                     time/num_lines,ori,0,ori_2,0);

        regenerated_nodes.push(node_parent);

    }

}

// Similar to create_nodes... This time we are recreating the tree for later
node_queue regenerate_nodes(frame &anim, node *root, 
                            std::unordered_map<char, std::string> bitmap){
    node_queue regenerated_nodes;

    // Performs Depth-first search and push back the priorirty queue
    depth_first_search(anim, root, regenerated_nodes, bitmap);
    
    return regenerated_nodes;
}

// Does a simple search to regenerated node_queue from root
void depth_first_search(frame &anim, node* root, node_queue &regenerated_nodes,
                        std::unordered_map<char, std::string> bitmap){

    // Are we on a leaf?
    if (!root->right && !root->left){
        if (root->key){
            std::string bitstring = bitmap[root->key];
            root->x = find_x_pos(anim, bitstring);
        }
        // Filling the regenerated nodes 
        regenerated_nodes.push(root);
    }

    if (root->right){
        depth_first_search(anim, root->right, regenerated_nodes, bitmap);
    }
    if (root->left){
        depth_first_search(anim, root->left, regenerated_nodes, bitmap);
    }

}

// Function to draw all layers
void draw_layers(std::vector<frame> layer){
    std::string pngid, number;
    for (size_t i = 0; i < num_frames; ++i){
        for (size_t j = layer.size() - 1; j > 0; --j){
            cairo_set_source_surface(layer[0].frame_ctx[i], 
                                     layer[j].frame_surface[i], 0, 0);
            cairo_paint(layer[0].frame_ctx[i]);
        }

        // Setting up number with stringstream
        std::stringstream ss;
        ss << std::setw(5) << std::setfill('0') << i;
        number = ss.str();

        pngid = layer[0].pngbase + number + ".png";
        std::cout << pngid << '\n';
        cairo_surface_write_to_png(layer[0].frame_surface[i], pngid.c_str());
    }

}
