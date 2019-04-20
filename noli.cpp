#include <iostream>
#include <cstdlib>
#include <vector>

#include <termbox.h>

typedef enum NeuronState
{
    NEURON_OFF  = 0,
    NEURON_ON   = 1,
    NEURON_RCVR = 2
} NeuronState;

class Synapse
{
    public:
    Synapse()
    {
        strength = 1;
        
        state = 0;
        state_next = 0;
        state_last = 0;
        
        stp = 1;
        stp_min = 1;
        stp_max = 1;
        stp_mod = 0;
        ltp_rtn = 0;
        
        ltp = 1;
        ltp_min = 1;
        ltp_max = 1;
        ltp_mod = 0;
        ltp_rtn = 0;
    }
    
    ~Synapse()
    {
    }
    
    void    propagate()
    {
        // Do nothing right now
    }
    
    void    commit()
    {
        state_last = state;
        state = state_next * strength * stp * ltp;
        
        stp = (stp > 1) ? stp - stp_rtn : stp + stp_rtn;
        ltp = (ltp > 1) ? ltp - ltp_rtn : ltp + ltp_rtn;
    }
    
    void    push(float signal)
    {
        state_next = signal;
    }
    
    float   pull()
    {
        return state;
    }
    
    float   delta()
    {
        return state - state_last;
    }
    
    void    adjust(bool fired)
    {
        stp = (fired) ? stp + stp_mod : stp - stp_mod;
        
        if(stp > stp_max)
        {
            stp = stp_max;
            ltp += ltp_mod;
            
            if(ltp > ltp_max)
                ltp = ltp_max;
        }

        if(stp < stp_min)
        {
            stp = stp_min;
            ltp -= ltp_mod;
            
            if(ltp < ltp_min)
                ltp = ltp_min;
        }        
    }
    
    float   state, state_next, state_last;
    float   strength;
    float   stp, stp_min, stp_max, stp_mod, stp_rtn;
    float   ltp, ltp_min, ltp_max, ltp_mod, ltp_rtn;
};

class Neuron
{
    public:
    Neuron(float n_thresh, float n_off, float n_on)
    {
        threshold = n_thresh;
        
        value_off = n_off;
        value_on = n_on;
        
        state = NEURON_OFF;
    }
    
    ~Neuron()
    {
    }
    
    void    propagate()
    {
        float trigger = 0, delta = -10000;
        Synapse* max_delta;
        
        for(int i=0;i<inputs.size();i++)
        {
            trigger += inputs[i]->pull();
            
            if(inputs[i]->delta() > delta)
            {
                delta = inputs[i]->delta();
                max_delta = inputs[i];
            }
        }

        switch(state)
        {
            case(NEURON_OFF):
            {
                if(trigger > threshold)
                {
                    state = NEURON_ON;
                    
                    for(int i=0;i<inputs.size();++i)
                    {
                        if(inputs[i] == max_delta)
                            inputs[i]->adjust(true);
                        else
                            inputs[i]->adjust(false);
                    }
                    
                    for(int i=0;i<outputs.size();i++)
                    {
                        outputs[i]->push(value_on);
                    }
                }
            }
            break;
            
            case(NEURON_ON):
            {
                state = NEURON_RCVR;
                
                for(int i=0;i<outputs.size();i++)
                {
                    outputs[i]->push(value_off);
                }
            }
            break;
            
            case(NEURON_RCVR):
            {
                state = NEURON_OFF;
            }
            break;
            
            default:
            {
                state = NEURON_OFF;
            }
            break;
        }
        
        // Make the synapses work
        for(int i=0;i<outputs.size();i++)
            outputs[i]->propagate();
    }
    
    void    commit()
    {
        // Make the synapses work
        for(int i=0;i<outputs.size();i++)
            outputs[i]->commit();
    }
    
    float   value_off, value_on;
    float   threshold;
    
    NeuronState state;
    
    std::vector<Synapse*> inputs;
    std::vector<Synapse*> outputs;
    
};    


int main(int argc, char** argv)
{
    std::vector<Neuron*> neurons;
    
    // Make random neurons
    for(int i=0;i<15;i++)
    {
        neurons.push_back(new Neuron((rand()%200/100.0), 0, (rand()%200)/100.0));
        std::cout << "Neuron " << i << ": thresh " << neurons[i]->threshold << ", off " << neurons[i]->value_off << ", on " << neurons[i]->value_on << "\n";
    }
    
    std::cout << "\n";
    
    // Make random connections between the neurons
    for(int i=0;i<15;i++)
    {
        int n_synapses = (rand() % 11) + 1;
        
        std::cout << "Neuron " << i << ":\n";
        
        for(int j=0;j<n_synapses;j++)
        {
            Synapse* n_synapse = new Synapse();
            int d_synapse;
            do { d_synapse = (rand() % 15); } while(d_synapse == i);
            
            neurons[i]->outputs.push_back(n_synapse);
            neurons[d_synapse]->inputs.push_back(n_synapse);
            
            std::cout << "  Synapse " << j << " connects to neuron " << d_synapse << "\n";
        }
    }
    
    std::cout << "\n";
    
    tb_init();
    tb_select_output_mode(TB_OUTPUT_GRAYSCALE);
    tb_event* event = new tb_event;
    
    neurons[4]->threshold = -1;
    neurons[4]->value_on = 5;
    
    bool running = true;
    
    while(running)
    {
        for(int i=0;i<15;i++)
        {
            neurons[i]->propagate();
            
            for(int d=0;d<22;d++)
                tb_change_cell(i, d+1, (tb_cell_buffer()+(sizeof(tb_cell)*((tb_width()*d)+i)))->ch, 22-d, 0);
                
            tb_change_cell(i, 0, ((neurons[i]->state == NEURON_OFF) ? 'O' : (neurons[i]->state == NEURON_ON) ? 'F' : 'R'), 23, 0);
        }
        
        for(int i=0;i<15;i++)
            neurons[i]->commit();
        
        tb_present();
        
        if(tb_peek_event(event, 500) != 0)
        {
            if(event->ch == 'q')
                running = false;
        }
        
    }
    
    tb_shutdown();
}



















