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
 * in basic_block without header modifications or friend accessors.
 */
#define protected public
#include <gnuradio/basic_block.h>
#undef protected

/*!
 * Allow access to buffer members to point to a custom location.
 */
#define private public
#define protected public
#include <gnuradio/buffer.h>
#undef private
#undef protected

#include <gnuradio/block.h>
#include <gnuradio/block_detail.h>

#include "pothos_block_executor.h"
#include "pothos_support.h"
#include <cmath>
#include <cassert>
#include <iostream>

/***********************************************************************
 * Dummy block to store produced messages
 **********************************************************************/
class MsgAcceptBlock : public gr::block
{
public:
    MsgAcceptBlock(const std::string &name):
        gr::block(
            "MsgAcceptBlock["+name+"]",
            gr::io_signature::make(0, 0, 0),
            gr::io_signature::make(0, 0, 0))
    {
        return;
    }
};

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
    boost::shared_ptr<gr::block> d_msg_accept_block;
    boost::shared_ptr<gr::block> d_block;
    std::vector<uint64_t> d_last_input_tag_offset;
    gr::pothos_block_executor *d_exec;
    gr::block_detail_sptr d_detail;
};

/***********************************************************************
 * init the name and ports -- called by the block constructor
 **********************************************************************/
GrPothosBlock::GrPothosBlock(boost::shared_ptr<gr::block> block):
    d_msg_accept_block(new MsgAcceptBlock(block->name())),
    d_block(block)
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

        //subscribe the dummy message acceptor block to hold output messages
        d_msg_accept_block->message_port_register_in(port_id);
        d_block->message_port_sub(port_id, pmt::cons(d_msg_accept_block->alias_pmt(), port_id));
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
    d_detail = gr::make_block_detail(this->inputs().size(), this->outputs().size());
    d_block->set_detail(d_detail);

    //pick a small default that will successfully allocate
    //the actual size is filled in later inside of work()
    static const size_t defaultBufSize(1024);

    //load detail inputs with dummy buffers to store state
    d_last_input_tag_offset.resize(d_detail->ninputs());
    for (int i = 0; i < d_detail->ninputs(); i++)
    {
        const auto buff = gr::make_buffer(defaultBufSize, this->input(i)->dtype().size());
        const auto reader = gr::buffer_add_reader(buff, 0);
        d_detail->set_input(i, reader);
        d_last_input_tag_offset[i] = this->input(i)->totalElements();
    }

    //load detail outputs with dummy buffers to store state
    for (int i = 0; i < d_detail->noutputs(); i++)
    {
        const auto buff = gr::make_buffer(defaultBufSize, this->output(i)->dtype().size());
        d_detail->set_output(i, buff);
    }

    auto block = gr::cast_to_block_sptr(d_block->shared_from_this());
    d_exec = new gr::pothos_block_executor(block);
}

void GrPothosBlock::deactivate(void)
{
    d_detail.reset();
    d_block->set_detail(d_detail);
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

    //handle any queued up messages
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

    //propagate output messages produced thus far
    for (const auto &i : d_msg_accept_block->get_msg_map())
    {
        while((msg = d_msg_accept_block->delete_head_nowait(i.first)))
        {
            this->output(pmt::symbol_to_string(i.first))->postMessage(pmt_to_obj(msg));
        }
    }

    //no streaming ports, there is nothing to do in the logic below
    if (d_detail->noutputs() == 0 and d_detail->ninputs() == 0) return;

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

    //force buffer to look at current port's resources
    for (auto port : this->inputs())
    {
        auto &last_label_offset = d_last_input_tag_offset[port->index()];
        const auto reader = d_detail->input(port->index());
        const auto buff = reader->buffer();

        buff->d_base = port->buffer().as<char *>();
        buff->d_bufsize = port->elements();
        buff->d_write_index = port->elements();
        reader->d_read_index = 0;
        reader->d_abs_read_offset = port->totalElements();

        //copy input labels into the input buffer's tags
        uint64_t new_last_label_offset = 0;
        for (const auto &label : port->labels())
        {
            auto offset = label.index + port->totalElements();
            if (offset < last_label_offset) continue;
            new_last_label_offset = offset;
            gr::tag_t tag;
            tag.key = pmt::string_to_symbol(label.id);
            tag.value = obj_to_pmt(label.data);
            tag.offset = offset;
            buff->add_item_tag(tag);
        }

        //update the label propagation stopper to this point
        if (new_last_label_offset != 0)
            last_label_offset = new_last_label_offset;
    }

    //force buffer to look at current port's resources
    for (auto port : this->outputs())
    {
        const auto buff = d_detail->output(port->index());
        buff->d_base = port->buffer().as<char *>();
        buff->d_bufsize = port->elements();
        buff->d_write_index = 0;
        buff->d_abs_write_offset = port->totalElements();
    }

    //run the executor for one iteration to call into derived class's work()
    d_exec->run_one_iteration();

    //search the detail input buffer for consume
    for (auto port : this->inputs())
    {
        const auto nread = d_detail->nitems_read(port->index());
        port->consume(nread-port->totalElements());
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

        //remove all tags once posted
        buff->d_item_tags.clear();
    }

    //propagate output messages produced from work
    for (const auto &i : d_msg_accept_block->get_msg_map())
    {
        while((msg = d_msg_accept_block->delete_head_nowait(i.first)))
        {
            this->output(pmt::symbol_to_string(i.first))->postMessage(pmt_to_obj(msg));
        }
    }
}

/***********************************************************************
 * do not propagate labels here - the executor handles it
 **********************************************************************/
void GrPothosBlock::propagateLabels(const Pothos::InputPort *)
{
    return;
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
