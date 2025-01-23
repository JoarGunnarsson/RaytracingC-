#include "medium.h"


Medium::Medium(const vec3& _scattering_albedo, const vec3& _absorption_albedo) : 
scattering_albedo(_scattering_albedo), absorption_albedo(_absorption_albedo) {
    extinction_albedo = absorption_albedo + scattering_albedo;
}

double Medium::sample_distance() const{
    return constants::max_ray_distance;
}

vec3 Medium::sample_direction(const vec3& incident_vector) const{
    return sample_spherical();
}

vec3 Medium::transmittance_albedo(const double distance) const{
    return exp_vector(-extinction_albedo * distance);
}

void Medium::Integrate(Object** objects, const int number_of_objects, Ray& incoming_ray, vec3& Lv, vec3& transmittance, vec3& weight, Ray& outgoing_ray) const{
    Hit hit;
    if(!find_closest_hit(hit, incoming_ray, objects, number_of_objects)){
        return;
    }

    transmittance = colors::WHITE;
}

vec3 Medium::sample(Object** objects, const int number_of_objects, const double distance, const bool scatter) const{
    return colors::WHITE;
}

vec3 Medium::sample_direct(const vec3& scattering_point, Object** objects, const int number_of_objects, const MediumStack& current_medium_stack) const{
    return colors::BLACK;
}


BeersLawMedium::BeersLawMedium(const vec3& _scattering_albedo, const vec3& _absorption_albedo) : Medium(0, _absorption_albedo){}

void BeersLawMedium::Integrate(Object** objects, const int number_of_objects, Ray& incoming_ray, vec3& Lv, vec3& transmittance, vec3& weight, Ray& outgoing_ray) const{
    Hit hit;
    if(!find_closest_hit(hit, incoming_ray, objects, number_of_objects)){
        return;
    }

    transmittance = transmittance_albedo((hit.intersection_point - incoming_ray.starting_position).length());
    weight = 1;
    outgoing_ray = incoming_ray;
}

vec3 BeersLawMedium::sample(Object** object, const int number_of_objects, const double distance, const bool scatter) const{
    return transmittance_albedo(distance);
}


double ScatteringMediumHomogenous::sample_distance() const{
    int channel = random_int(0, 3);
    return -std::log(1 -random_uniform(0, 1)) / extinction_albedo[channel];
}

vec3 ScatteringMediumHomogenous::sample(Object** objects, const int number_of_objects, const double distance, const bool scatter) const{
    vec3 tr = transmittance_albedo(distance);
    vec3 density = scatter ? extinction_albedo * tr : tr;
    double pdf = 0;
    for (int i = 0; i < 3; i++){
        pdf += density[i];
    }
    pdf *= 1.0 / 3.0;
    return scatter ? tr * scattering_albedo / pdf : tr / pdf;
}

vec3 ScatteringMediumHomogenous::sample_direct(const vec3& scattering_point, Object** objects, const int number_of_objects, const MediumStack& current_medium_stack) const{
    vec3 sampled_direction; 
    return direct_lighting(scattering_point, objects, number_of_objects, sampled_direction, current_medium_stack) * 0.25 / M_PI;
}


MediumStack::MediumStack(){ stack_size = 0; }
MediumStack::MediumStack(Medium** initial_array, const int size){
    stack_size = 0;
    for (int i = 0; i < size; i++){
        add_medium(initial_array[i], initial_array[i] -> id);
    }
}

MediumStack::~MediumStack(){
    delete[] medium_array;
}

Medium** MediumStack::get_array() const { return medium_array; }
int MediumStack::get_stack_size() const { return stack_size; }

Medium* MediumStack::get_medium() const{
    if(stack_size == 0){
        return nullptr;
    }
    
    return medium_array[stack_size-1];
}

void MediumStack::add_medium(Medium* medium, const int id){
    // Call this when entering a new medium.
    //std::cout << "Add! Size (before addition is made): " << stack_size << ", id: " << id << "\n";
    if (stack_size == MAX_STACK_SIZE){
        throw std::invalid_argument("Cannot add another medium to stack, stack is full!");
    }
    bool found = false;
    for (int i = 0; i < stack_size; i++){
        if (medium_array[i] -> id == id){
            //std::cout << "Error: 1. Found a medium with same ID!\n"; 
            return;
        }
    }
    
    medium -> id = id;
    medium_array[stack_size] = medium;
    stack_size++;
}

void MediumStack::pop_medium(const int id){
    // Call this when exiting a medium.
    //std::cout << "Remove! Size (before addition is made): " << stack_size << ", id: " << id << "\n";
    for (int i = stack_size-1; i >= 0; i--){
        if (medium_array[i] -> id == id){
            medium_array[i] = nullptr;
            stack_size--;
            return;
        }
    }
    //std::cout << "Error: 2. Could not remove medium from stack because no matching medium was found!\n";
    //throw std::invalid_argument("Could not remove medium from stack because no matching medium was found!");
}
