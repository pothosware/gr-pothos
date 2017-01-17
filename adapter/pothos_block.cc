/*
 * Copyright 2014-2017 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

/*!
 * Allow access to has_msg_handler and dispatch_msg
 * in basic_block and the detail structure of block
 * without header modifications or friend accessors.
 */
#define protected public
#include <gnuradio/basic_block.h>
#undef protected

#define private public
#include <gnuradio/block.h>
#undef private

#include <gnuradio/block_detail.h>
#include <gnuradio/buffer.h>

#include "pothos_block_executor.h"
#include "pothos_support.h"
#include <cmath>
#include <cassert>
#include <cctype>
#include <iostream>

/***********************************************************************
 * try our best to infer the data type given the info at hand
 **********************************************************************/
static Pothos::DType inferDType(const size_t ioSize, const std::string &name, const bool isInput)
{
    Pothos::DType dtype("custom", ioSize); //default if we cant figure it out

    //make a guess based on size and the usual types
    if ((ioSize%sizeof(gr_complex)) == 0) dtype = Pothos::DType(typeid(gr_complex), ioSize/sizeof(gr_complex));
    else if ((ioSize%sizeof(float)) == 0) dtype = Pothos::DType(typeid(float), ioSize/sizeof(float));
    else if ((ioSize%sizeof(short)) == 0) dtype = Pothos::DType(typeid(short), ioSize/sizeof(short));
    else if ((ioSize%sizeof(char)) == 0) dtype = Pothos::DType(typeid(char), ioSize/sizeof(char));

    const auto lastUnder = name.find_last_of("_");
    if (lastUnder == std::string::npos) return dtype;

    //grab data type suffix
    auto suffix = name.substr(lastUnder+1);
    if (suffix.empty()) return dtype;
    if (suffix == "sink") return dtype;
    if (suffix == "source") return dtype;

    //strip leading v (for vector)
    if (std::tolower(suffix.front()) == 'v') suffix = suffix.substr(1);
    if (suffix.empty()) return dtype;

    //extract signature character
    char sig = suffix.front();
    if (not isInput and suffix.size() >= 2) sig = suffix.at(1);
    sig = std::tolower(sig);

    //inspect signature and io type for size-multiple
    #define inspectSig(sigName, sigType) \
        if (sig == sigName and ioSize%sizeof(sigType) == 0) \
            return Pothos::DType(typeid(sigType), ioSize/sizeof(sigType))
    inspectSig('b', char);
    inspectSig('s', short);
    inspectSig('i', int);
    inspectSig('f', float);
    inspectSig('c', gr_complex);
    return dtype;
}

/***********************************************************************
 * GrPothosBlock interfaces a gr::basic_block to the Pothos framework 
 **********************************************************************/
class GrPothosBlock : public Pothos::Block
{
public:
    static Pothos::Block *make(boost::shared_ptr<gr::block> block)
    {
        return new GrPothosBlock(block);
    }

    GrPothosBlock(boost::shared_ptr<gr::block> block);
    ~GrPothosBlock(void);
    void __setNumInputs(size_t);
    void __setNumOutputs(size_t);
    void __setInputAlias(const std::string &, const std::string &);
    void __setOutputAlias(const std::string &, const std::string &);
    void activate(void);
    void deactivate(void);
    void work(void);
    void propagateLabels(const Pothos::InputPort *input);
    Pothos::BufferManager::Sptr getInputBufferManager(const std::string &name, const std::string &domain);
    Pothos::BufferManager::Sptr getOutputBufferManager(const std::string &name, const std::string &domain);

private:
    boost::shared_ptr<gr::block> d_block;
    gr::pothos_block_executor *d_exec;
    gr::block_detail_sptr &d_detail;
};

/***********************************************************************
 * init the name and ports -- called by the block constructor
 **********************************************************************/
GrPothosBlock::GrPothosBlock(boost::shared_ptr<gr::block> block):
    d_block(block),
    d_detail(block->d_detail)
{
    Pothos::Block::setName(d_block->name());

    auto d_input_signature = d_block->input_signature();
    for (size_t i = 0; i < std::max<size_t>(d_input_signature->min_streams(), d_input_signature->sizeof_stream_items().size()); i++)
    {
        if (d_input_signature->max_streams() != gr::io_signature::IO_INFINITE and int(i) >= d_input_signature->max_streams()) break;
        auto bytes = d_input_signature->sizeof_stream_item(i);
        Pothos::Block::setupInput(i, inferDType(bytes, d_block->name(), true));
    }

    auto d_output_signature = d_block->output_signature();
    for (size_t i = 0; i < std::max<size_t>(d_output_signature->min_streams(), d_output_signature->sizeof_stream_items().size()); i++)
    {
        if (d_output_signature->max_streams() != gr::io_signature::IO_INFINITE and int(i) >= d_output_signature->max_streams()) break;
        auto bytes = d_output_signature->sizeof_stream_item(i);
        Pothos::Block::setupOutput(i, inferDType(bytes, d_block->name(), false));
    }

    pmt::pmt_t msg_ports_in = d_block->message_ports_in();
    for (size_t i = 0; i < pmt::length(msg_ports_in); i++)
    {
        auto port_id = pmt::vector_ref(msg_ports_in, i);
        if (pmt::symbol_to_string(port_id) == "system") continue; //ignore ubiquitous system port
        this->setupInput(pmt::symbol_to_string(port_id));
    }

    pmt::pmt_t msg_ports_out = d_block->message_ports_out();
    for (size_t i = 0; i < pmt::length(msg_ports_out); i++)
    {
        auto port_id = pmt::vector_ref(msg_ports_out, i);
        this->setupOutput(pmt::symbol_to_string(port_id));
    }

    Pothos::Block::registerCall(this, POTHOS_FCN_TUPLE(GrPothosBlock, __setNumInputs));
    Pothos::Block::registerCall(this, POTHOS_FCN_TUPLE(GrPothosBlock, __setNumOutputs));
    Pothos::Block::registerCall(this, POTHOS_FCN_TUPLE(GrPothosBlock, __setInputAlias));
    Pothos::Block::registerCall(this, POTHOS_FCN_TUPLE(GrPothosBlock, __setOutputAlias));
}

GrPothosBlock::~GrPothosBlock(void)
{
    return;
}

void GrPothosBlock::__setNumInputs(const size_t num)
{
    for (size_t i = Pothos::Block::inputs().size(); i < num; i++)
    {
        assert(not Pothos::Block::inputs().empty());
        Pothos::Block::setupInput(i, Pothos::Block::inputs().back()->dtype());
    }
}

void GrPothosBlock::__setNumOutputs(const size_t num)
{
    for (size_t i = Pothos::Block::outputs().size(); i < num; i++)
    {
        assert(not Pothos::Block::outputs().empty());
        Pothos::Block::setupOutput(i, Pothos::Block::outputs().back()->dtype());
    }
}

void GrPothosBlock::__setInputAlias(const std::string &name, const std::string &alias)
{
    this->input(name)->setAlias(alias);
}

void GrPothosBlock::__setOutputAlias(const std::string &name, const std::string &alias)
{
    this->output(name)->setAlias(alias);
}

/***********************************************************************
 * activation/deactivate notification events
 **********************************************************************/
void GrPothosBlock::activate(void)
{
    //create block detail to handle produce/consume, totals, and tags
    //detail is made late because some blocks late allocate ports
    //detail maintains a global count that matches the port totalElements
    //and therefore its never cleaned up.
    if (not d_detail)
    {
        d_detail = gr::make_block_detail(this->inputs().size(), this->outputs().size());

        //load detail inputs with dummy buffers to store state
        for (int i = 0; i < d_detail->ninputs(); i++)
        {
            const auto buff = gr::make_buffer(1024, 1);
            const auto reader = gr::buffer_add_reader(buff, 0);
            d_detail->set_input(i, reader);
        }

        //load detail outputs with dummy buffers to store state
        for (int i = 0; i < d_detail->noutputs(); i++)
        {
            const auto buff = gr::make_buffer(1024, 1);
            d_detail->set_output(i, buff);
        }
    }

    auto block = gr::cast_to_block_sptr(d_block->shared_from_this());
    d_exec = new gr::pothos_block_executor(block);
}

void GrPothosBlock::deactivate(void)
{
    delete d_exec;
}

/***********************************************************************
 * stimulus even occured - handle inputs and call general work
 **********************************************************************/
void GrPothosBlock::work(void)
{
    //forward messages into the queues
    for (const auto &pair : Pothos::Block::allInputs())
    {
        auto inputPort = pair.second;
        while (inputPort->hasMessage()) d_block->_post(
            pmt::string_to_symbol(inputPort->name()),
            obj_to_pmt(inputPort->popMessage()));
    }

    // handle any queued up messages
    pmt::pmt_t msg;
    for (const auto &i : d_block->get_msg_map())
    {
        // Check if we have a message handler attached before getting
        // any messages. This is mostly a protection for the unknown
        // startup sequence of the threads.
        if(d_block->has_msg_handler(i.first)) {
          while((msg = d_block->delete_head_nowait(i.first))) {
            d_block->dispatch_msg(i.first,msg);
          }
        }
    }

    //re-apply reserve in-case it changed (low cost setter)
    size_t reserve = d_block->history();
    if (d_block->fixed_rate())
    {
        const size_t noutput_items = d_block->output_multiple_set()?d_block->output_multiple():1;
        reserve = d_block->fixed_rate_noutput_to_ninput(noutput_items);
    }
    for (auto input : Pothos::Block::inputs()) input->setReserve(reserve);

    //check that input and output items meets the reserve req
    const auto &workInfo = Pothos::Block::workInfo();
    if (workInfo.minInElements < reserve) return;
    if (workInfo.minOutElements == 0) return;
    if (d_block->fixed_rate() and int(workInfo.minOutElements) < d_block->fixed_rate_ninput_to_noutput(reserve)) return;

    //copy input labels into the input buffer's tags
    for (auto port : this->inputs())
    {
        const auto buff = d_detail->input(port->index())->buffer();
        for (const auto &label : port->labels())
        {
            auto offset = label.index + port->totalElements();
            gr::tag_t tag;
            tag.key = pmt::string_to_symbol(label.id);
            tag.value = obj_to_pmt(label.data);
            tag.offset = offset;
            buff->add_item_tag(tag);
        }
    }

    //run the executor for one iteration to call into derived class's work()
    d_exec->run_one_iteration(this, d_detail);

    //search the detail input buffer for consume
    for (auto port : this->inputs())
    {
        const auto nread = d_detail->nitems_read(port->index());
        port->consume(nread-port->totalElements());

        //these tags were either used or will be added next run
        const auto buff = d_detail->input(port->index())->buffer();
        buff->prune_tags(nread);
    }

    //search the detail output buffer for produce
    for (auto port : this->outputs())
    {
        const auto nwritten = d_detail->nitems_written(port->index());
        port->produce(nwritten-port->totalElements());

        //post output labels from output buffer's tags
        const auto buff = d_detail->output(port->index());
        for (auto it = buff->get_tags_begin(); it != buff->get_tags_end(); it++)
        {
            const auto &tag = it->second;
            Pothos::Label label;
            label.id = pmt::symbol_to_string(tag.key);
            label.data = pmt_to_obj(tag.value);
            assert(tag.offset >= port->totalElements());
            label.index = tag.offset - port->totalElements();
            port->postLabel(label);
        }
        buff->prune_tags(nwritten);
    }
}

/***********************************************************************
 * propagateLabels
 **********************************************************************/
void GrPothosBlock::propagateLabels(const Pothos::InputPort *inputPort)
{
    switch (d_block->tag_propagation_policy())
    {
    case gr::block::TPP_DONT: return;
    case gr::block::TPP_ONE_TO_ONE:
    {
        if (inputPort->index() == -1) return;
        const auto portIndex = size_t(inputPort->index());
        if (portIndex >= Pothos::Block::outputs().size()) return;
        auto outputPort = Pothos::Block::output(portIndex);
        for (const auto &label : inputPort->labels())
        {
            auto newLabel = label;
            newLabel.index += d_block->sample_delay(inputPort->index());
            newLabel.index = std::llround(newLabel.index*d_block->relative_rate());
            outputPort->postLabel(label);
        }
    }
    case gr::block::TPP_ALL_TO_ALL:
    {
        for (const auto &label : inputPort->labels())
        {
            auto newLabel = label;
            newLabel.index += d_block->sample_delay(inputPort->index());
            newLabel.index = std::llround(newLabel.index*d_block->relative_rate());
            for (auto outputPort : Pothos::Block::outputs()) outputPort->postLabel(label);
        }
    }
    default: return;
    }
}

/***********************************************************************
 * custom buffer managers - circular buffers for blocks with history
 **********************************************************************/
Pothos::BufferManager::Sptr GrPothosBlock::getInputBufferManager(const std::string &name, const std::string &domain)
{
    //install circular buffer when history is enabled
    const size_t d_history = d_block->history();
    if (d_history > 1)
    {
        Pothos::BufferManagerArgs args;
        const auto require = (d_history+1)*Pothos::Block::input(name)->dtype().size();
        args.bufferSize = std::max(args.bufferSize, require*8/*factor*/);
        return Pothos::BufferManager::make("circular", args);
    }
    return Pothos::Block::getInputBufferManager(name, domain);
}

Pothos::BufferManager::Sptr GrPothosBlock::getOutputBufferManager(const std::string &name, const std::string &domain)
{
    return Pothos::Block::getOutputBufferManager(name, domain);
}

/***********************************************************************
 * registration
 **********************************************************************/
static Pothos::BlockRegistry registerGrPothosBlock(
    "/gnuradio/block", &GrPothosBlock::make);
