#include "eqp_profiler.h"
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <algorithm>

EQP::CPU::ST::Profiler st_profiler;
EQP::CPU::MT::Profiler mt_profiler;

struct EQP::CPU::MT::Profiler::impl
{
	std::mutex lock_;
	std::unordered_map<std::thread::id, ThreadInfo*> nodes_;
};

EQP::CPU::ST::Profiler &EQP::CPU::ST::GetProfiler() {
	return st_profiler;
}

EQP::CPU::MT::Profiler &EQP::CPU::MT::GetProfiler() {
	return mt_profiler;
}

EQP::CPU::ST::Profiler::Profiler() {
	root_ = new ProfilerNode;
	root_->SetParent(root_);
	current_ = root_;
}

EQP::CPU::ST::Profiler::~Profiler() {
	delete root_;
}

void EQP::CPU::ST::Profiler::EventStarted(const char *func, const char *name) {
	std::string cur_name = func;
	if(name) {
		cur_name += " - ";
		cur_name += name;
	}

	auto search = current_->GetNodes().find(cur_name);
	if(search != current_->GetNodes().end()) {
		current_ = search->second;
	} else {
		ProfilerNode *t = new ProfilerNode;
		t->SetParent(current_);
		current_->GetNodes()[cur_name] = t;
		current_ = t;
	}
}

void EQP::CPU::ST::Profiler::EventFinished(uint64_t time) {
	current_->GetTime() += time;
	current_->GetCount()++;
	current_ = current_->GetParent();
}

void EQP::CPU::ST::Profiler::Clear() {
	for(auto &iter : root_->GetNodes()) {
		delete iter.second;
	}
	root_->GetNodes().clear();

	root_->SetTime(0);
	root_->SetCount(0);
	current_ = root_;
}

void EQP::CPU::ST::Profiler::Dump(std::ostream &stream, int num) {
	uint64_t total = 0;
	std::vector<ProfilerNodeDump> sorted_vec;
	sorted_vec.reserve(root_->GetNodes().size() + 1);

	for(auto &iter : root_->GetNodes()) {
		ProfilerNodeDump n;
		n.name = iter.first;
		n.node = iter.second;
		sorted_vec.push_back(n);

		total += iter.second->GetTime();
	}

	std::sort(sorted_vec.begin(), sorted_vec.end(), 
			  [](const ProfilerNodeDump& a, const ProfilerNodeDump& b) { return a.node->GetTime() > b.node->GetTime(); });

	int i = 0;
	for(auto &iter : sorted_vec) {
		if(num > 0 && i >= num) {
			break;
		}

		iter.node->Dump(stream, iter.name, total, 0, num);
		++i;
	}
}

EQP::CPU::MT::Profiler::ThreadInfo::ThreadInfo() {
	root_ = new ProfilerNode;
	root_->SetParent(root_);
	current_ = root_;
}

EQP::CPU::MT::Profiler::ThreadInfo::~ThreadInfo() {
	delete root_;
}

EQP::CPU::MT::Profiler::Profiler() {
	imp_ = new impl;
}

EQP::CPU::MT::Profiler::~Profiler() {
	delete imp_;
}

void EQP::CPU::MT::Profiler::EventStarted(const char *func, const char *name) {
	std::string cur_name = func;
	if(name) {
		cur_name += " - ";
		cur_name += name;
	}

	ThreadInfo *ti = nullptr;
	imp_->lock_.lock();
	auto ti_search = imp_->nodes_.find(std::this_thread::get_id());
	if(ti_search == imp_->nodes_.end()) {
		ti = new ThreadInfo;
		imp_->nodes_[std::this_thread::get_id()] = ti;
	} else {
		ti = ti_search->second;
	}
	imp_->lock_.unlock();

	auto search = ti->current_->GetNodes().find(cur_name);
	if(search != ti->current_->GetNodes().end()) {
		ti->current_ = search->second;
	}
	else {
		ProfilerNode *t = new ProfilerNode;
		t->SetParent(ti->current_);
		ti->current_->GetNodes()[cur_name] = t;
		ti->current_ = t;
	}
}

void EQP::CPU::MT::Profiler::EventFinished(uint64_t time) {
	ThreadInfo *ti = nullptr;
	imp_->lock_.lock();
	auto ti_search = imp_->nodes_.find(std::this_thread::get_id());
	if(ti_search == imp_->nodes_.end()) {
		imp_->lock_.unlock();
		return;
	}
	else {
		ti = ti_search->second;
	}
	imp_->lock_.unlock();

	ti->current_->GetTime() += time;
	ti->current_->GetCount()++;
	ti->current_ = ti->current_->GetParent();
}

void EQP::CPU::MT::Profiler::Clear() {
	imp_->lock_.lock();
	for(auto &iter : imp_->nodes_) {
		delete iter.second;
	}

	imp_->nodes_.clear();
	imp_->lock_.unlock();
}

void EQP::CPU::MT::Profiler::Dump(std::ostream &stream, int num) {
	imp_->lock_.lock();
	for(auto &iter : imp_->nodes_) {
		stream << "Thread: " << iter.first << std::endl;
		uint64_t total = 0;
		std::vector<ProfilerNodeDump> sorted_vec;
		sorted_vec.reserve(iter.second->root_->GetNodes().size() + 1);

		for(auto &t_iter : iter.second->root_->GetNodes()) {
			ProfilerNodeDump n;
			n.name = t_iter.first;
			n.node = t_iter.second;
			sorted_vec.push_back(n);

			total += t_iter.second->GetTime();
		}
		
		std::sort(sorted_vec.begin(), sorted_vec.end(), 
				  [](const ProfilerNodeDump& a, const ProfilerNodeDump& b) { return a.node->GetTime() > b.node->GetTime(); });

		int i = 0;
		for(auto &t_iter : sorted_vec) {
			if(num > 0 && i >= num) {
				break;
			}
			t_iter.node->Dump(stream, t_iter.name, total, 1, num);
			++i;
		}

		stream << std::endl;
	}
	imp_->lock_.unlock();
}

