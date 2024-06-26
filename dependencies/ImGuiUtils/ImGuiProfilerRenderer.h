#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <array>
#include <chrono>
#include <algorithm>

#include "ProfilerTask.h"

#include <glm/glm.hpp>
#include <imgui.h>

namespace ImGuiUtils
{
	glm::vec2 Vec2(ImVec2 vec)
	{
		return glm::vec2(vec.x, vec.y);
	}

	class ProfilerGraph
	{
	public:
		int frameWidth;
		int frameSpacing;
		bool useColoredLegendText;

		ProfilerGraph(size_t framesCount)
		{
			frames.resize(framesCount);
			for (auto& frame : frames) {
				frame.tasks.reserve(100);
			}

			frameWidth = 3;
			frameSpacing = 1;
			useColoredLegendText = false;
		}

		void LoadFrameData(const legit::ProfilerTask* tasks, size_t count)
		{
			auto& currFrame = frames[currFrameIndex];
			size_t taskIndex;

			currFrame.tasks.resize(0);
			for (taskIndex = 0; taskIndex < count; taskIndex++) {
				currFrame.tasks.push_back(tasks[taskIndex]);
				/*
				if (taskIndex == 0) {
					currFrame.tasks.push_back(tasks[taskIndex]);
				} else {
					if (tasks[taskIndex - 1].color != tasks[taskIndex].color || tasks[taskIndex - 1].name != tasks[taskIndex].name) {
						currFrame.tasks.push_back(tasks[taskIndex]);
					} else {
						currFrame.tasks.back().endTime = tasks[taskIndex].endTime;
					}
				}
				*/
			}
			currFrame.taskStatsIndex.resize(currFrame.tasks.size());

			for (taskIndex = 0; taskIndex < currFrame.tasks.size(); taskIndex++) {
				auto& task = currFrame.tasks[taskIndex];
				auto it = taskNameToStatsIndex.find(task.name);
				if (it == taskNameToStatsIndex.end()) {
					taskNameToStatsIndex[task.name] = taskStats.size();

					TaskStats taskStat;
					taskStat.maxTime = -1.0;
					taskStats.push_back(taskStat);
				}
				currFrame.taskStatsIndex[taskIndex] = taskNameToStatsIndex[task.name];
			}
			currFrameIndex = (currFrameIndex + 1) % frames.size();

			RebuildTaskStats(currFrameIndex, 300/*frames.size()*/);
		}

		void RenderTimings(int graphWidth, int legendWidth, int height, int frameIndexOffset)
		{
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			const glm::vec2 widgetPos = Vec2(ImGui::GetCursorScreenPos());
			RenderGraph(drawList, widgetPos, glm::vec2(graphWidth, height), frameIndexOffset);
			RenderLegend(drawList, widgetPos + glm::vec2(graphWidth, 0.0f), glm::vec2(legendWidth, height), frameIndexOffset);
			ImGui::Dummy(ImVec2(float(graphWidth + legendWidth), float(height)));
		}

		/*void bla()
		{

		}*/
	protected:
		void RebuildTaskStats(size_t endFrame, size_t framesCount)
		{
			for (auto& taskStat : taskStats)
			{
				taskStat.maxTime = -1.0f;
				taskStat.priorityOrder = size_t(-1);
				taskStat.onScreenIndex = size_t(-1);
			}

			for (size_t frameNumber = 0; frameNumber < framesCount; frameNumber++)
			{
				size_t frameIndex = (endFrame - 1 - frameNumber + frames.size()) % frames.size();
				auto& frame = frames[frameIndex];
				for (size_t taskIndex = 0; taskIndex < frame.tasks.size(); taskIndex++)
				{
					auto& task = frame.tasks[taskIndex];
					auto& stats = taskStats[frame.taskStatsIndex[taskIndex]];
					stats.maxTime = std::max(stats.maxTime, task.endTime - task.startTime);
				}
			}
			std::vector<size_t> statPriorities;
			statPriorities.resize(taskStats.size());
			for (size_t statIndex = 0; statIndex < taskStats.size(); statIndex++)
				statPriorities[statIndex] = statIndex;

			std::sort(statPriorities.begin(), statPriorities.end(), [this](size_t left, size_t right) {return taskStats[left].maxTime > taskStats[right].maxTime; });
			for (size_t statNumber = 0; statNumber < taskStats.size(); statNumber++)
			{
				size_t statIndex = statPriorities[statNumber];
				taskStats[statIndex].priorityOrder = statNumber;
			}
		}
		void RenderGraph(ImDrawList* drawList, glm::vec2 graphPos, glm::vec2 graphSize, size_t frameIndexOffset)
		{
			Rect(drawList, graphPos, graphPos + graphSize, 0xffffffff, false);

			float maxFrameTime = 1.0f / 30.0f;
			float heightThreshold = 1.0f;

			for (size_t frameNumber = 0; frameNumber < frames.size(); frameNumber++)
			{
				size_t frameIndex = (currFrameIndex - frameIndexOffset - 1 - frameNumber + 2 * frames.size()) % frames.size();

				glm::vec2 framePos = graphPos + glm::vec2(graphSize.x - 1 - frameWidth - (frameWidth + frameSpacing) * frameNumber, graphSize.y - 1);
				if (framePos.x < graphPos.x + 1)
					break;
				glm::vec2 taskPos = framePos + glm::vec2(0.0f, 0.0f);
				auto& frame = frames[frameIndex];
				for (auto task : frame.tasks)
				{
					float taskStartHeight = (float(task.startTime) / maxFrameTime) * graphSize.y;
					float taskEndHeight = (float(task.endTime) / maxFrameTime) * graphSize.y;
					//taskMaxCosts[task.name] = std::max(taskMaxCosts[task.name], task.endTime - task.startTime);
					if (abs(taskEndHeight - taskStartHeight) > heightThreshold)
						Rect(drawList, taskPos + glm::vec2(0.0f, -taskStartHeight), taskPos + glm::vec2(frameWidth, -taskEndHeight), task.color, true);
				}
			}
		}
		void RenderLegend(ImDrawList* drawList, glm::vec2 legendPos, glm::vec2 legendSize, size_t frameIndexOffset)
		{
			float markerLeftRectMargin = 3.0f;
			float markerLeftRectWidth = 5.0f;
			float maxFrameTime = 1.0f / 30.0f;
			float markerMidWidth = 30.0f;
			float markerRightRectWidth = 10.0f;
			float markerRigthRectMargin = 3.0f;
			float markerRightRectHeight = 10.0f;
			float markerRightRectSpacing = 4.0f;
			float nameOffset = 30.0f;
			glm::vec2 textMargin = glm::vec2(5.0f, -3.0f);

			auto& currFrame = frames[(currFrameIndex - frameIndexOffset - 1 + 2 * frames.size()) % frames.size()];
			size_t maxTasksCount = size_t(legendSize.y / (markerRightRectHeight + markerRightRectSpacing));

			for (auto& taskStat : taskStats)
			{
				taskStat.onScreenIndex = size_t(-1);
			}

			size_t tasksToShow = std::min<size_t>(taskStats.size(), maxTasksCount);
			size_t tasksShownCount = 0;
			for (size_t taskIndex = 0; taskIndex < currFrame.tasks.size(); taskIndex++)
			{
				auto& task = currFrame.tasks[taskIndex];
				auto& stat = taskStats[currFrame.taskStatsIndex[taskIndex]];

				if (stat.priorityOrder >= tasksToShow)
					continue;

				if (stat.onScreenIndex == size_t(-1))
				{
					stat.onScreenIndex = tasksShownCount++;
				}
				else
					continue;
				float taskStartHeight = (float(task.startTime) / maxFrameTime) * legendSize.y;
				float taskEndHeight = (float(task.endTime) / maxFrameTime) * legendSize.y;

				glm::vec2 markerLeftRectMin = legendPos + glm::vec2(markerLeftRectMargin, legendSize.y);
				glm::vec2 markerLeftRectMax = markerLeftRectMin + glm::vec2(markerLeftRectWidth, 0.0f);
				markerLeftRectMin.y -= taskStartHeight;
				markerLeftRectMax.y -= taskEndHeight;

				glm::vec2 markerRightRectMin = legendPos + glm::vec2(markerLeftRectMargin + markerLeftRectWidth + markerMidWidth, legendSize.y - markerRigthRectMargin - (markerRightRectHeight + markerRightRectSpacing) * stat.onScreenIndex);
				glm::vec2 markerRightRectMax = markerRightRectMin + glm::vec2(markerRightRectWidth, -markerRightRectHeight);
				RenderTaskMarker(drawList, markerLeftRectMin, markerLeftRectMax, markerRightRectMin, markerRightRectMax, task.color);

				uint32_t textColor = useColoredLegendText ? task.color : legit::Colors::imguiText;// task.color;

				float taskTimeMs = float(task.endTime - task.startTime);
				std::ostringstream timeText;
				timeText.precision(2);
				timeText << std::fixed << std::string("[") << (taskTimeMs * 1000.0f);

				Text(drawList, markerRightRectMax + textMargin, textColor, timeText.str().c_str());
				Text(drawList, markerRightRectMax + textMargin + glm::vec2(nameOffset, 0.0f), textColor, (std::string("ms] ") + task.name).c_str());
			}

			/*
			struct PriorityEntry
			{
			  bool isUsed;
			  legit::ProfilerTask task;
			};
			std::map<std::string, PriorityEntry> priorityEntries;
			for (auto priorityTask : priorityTasks)
			{
			  PriorityEntry entry;
			  entry.task = frames[priorityTask.frameIndex].tasks[priorityTask.taskIndex];
			  entry.isUsed = false;
			  priorityEntries[entry.task.name] = entry;
			}
			size_t shownTasksCount = 0;
			for (size_t taskIndex = 0; taskIndex < currFrame.tasks.size(); taskIndex++)
			{
			  auto &task = currFrame.tasks[taskIndex];
			  auto it = priorityEntries.find(task.name);
			  if (it != priorityEntries.end() && !it->second.isUsed)
			  {
				it->second.isUsed = true;

				float taskStartHeight = (float(task.startTime) / maxFrameTime) * legendSize.y;
				float taskEndHeight = (float(task.endTime) / maxFrameTime) * legendSize.y;


				glm::vec2 markerLeftRectMin = legendPos + glm::vec2(markerLeftRectMargin, legendSize.y);
				glm::vec2 markerLeftRectMax = markerLeftRectMin + glm::vec2(markerLeftRectWidth, 0.0f);
				markerLeftRectMin.y -= taskStartHeight;
				markerLeftRectMax.y -= taskEndHeight;

				glm::vec2 markerRightRectMin = legendPos + glm::vec2(markerLeftRectMargin + markerLeftRectWidth + markerMidWidth, legendSize.y - markerRigthRectMargin - (markerRightRectHeight + markerRightRectSpacing) * shownTasksCount);
				glm::vec2 markerRightRectMax = markerRightRectMin + glm::vec2(markerRightRectWidth, -markerRightRectHeight);
				RenderTaskMarker(drawList, markerLeftRectMin, markerLeftRectMax, markerRightRectMin, markerRightRectMax, task.color);

				uint32_t textColor = legit::Colors::imguiText;// task.color;

				float taskTimeMs = float(task.endTime - task.startTime);
				std::ostringstream timeText;
				timeText.precision(2);
				timeText << std::fixed << std::string("[") << (taskTimeMs * 1000.0f);

				Text(drawList, markerRightRectMax + textMargin, textColor, timeText.str().c_str());
				Text(drawList, markerRightRectMax + textMargin + glm::vec2(nameOffset, 0.0f), textColor, (std::string("ms] ") + task.name).c_str());
				shownTasksCount++;
			  }
			}*/

			/*for (size_t priorityTaskIndex = 0; priorityTaskIndex < priorityTasks.size(); priorityTaskIndex++)
			{
			  auto &priorityTask = priorityTasks[priorityTaskIndex];
			  auto &globalTask = frames[priorityTask.frameIndex].tasks[priorityTask.taskIndex];

			  size_t lastFrameTaskIndex = currFrame.FindTask(globalTask.name);

			  glm::vec2 taskPos = legendPos + marginSpacing + glm::vec2(0.0f, markerHeight) + glm::vec2(0.0f, (markerHeight + itemSpacing) * priorityTaskIndex);
			  Rect(drawList, taskPos, taskPos + glm::vec2(markerHeight, -markerHeight), task.color, true);
			  Text(drawList, taskPos + textOffset, 0xffffffff, task.name.c_str());
			}*/
		}

		static void Rect(ImDrawList* drawList, glm::vec2 minPoint, glm::vec2 maxPoint, uint32_t col, bool filled = true)
		{
			if (filled)
				drawList->AddRectFilled(ImVec2(minPoint.x, minPoint.y), ImVec2(maxPoint.x, maxPoint.y), col);
			else
				drawList->AddRect(ImVec2(minPoint.x, minPoint.y), ImVec2(maxPoint.x, maxPoint.y), col);
		}
		static void Text(ImDrawList* drawList, glm::vec2 point, uint32_t col, const char* text)
		{
			drawList->AddText(ImVec2(point.x, point.y), col, text);
		}
		static void Triangle(ImDrawList* drawList, std::array<glm::vec2, 3> points, uint32_t col, bool filled = true)
		{
			if (filled)
				drawList->AddTriangleFilled(ImVec2(points[0].x, points[0].y), ImVec2(points[1].x, points[1].y), ImVec2(points[2].x, points[2].y), col);
			else
				drawList->AddTriangle(ImVec2(points[0].x, points[0].y), ImVec2(points[1].x, points[1].y), ImVec2(points[2].x, points[2].y), col);
		}
		static void RenderTaskMarker(ImDrawList* drawList, glm::vec2 leftMinPoint, glm::vec2 leftMaxPoint, glm::vec2 rightMinPoint, glm::vec2 rightMaxPoint, uint32_t col)
		{
			Rect(drawList, leftMinPoint, leftMaxPoint, col, true);
			Rect(drawList, rightMinPoint, rightMaxPoint, col, true);
			std::array<ImVec2, 4> points = {
			  ImVec2(leftMaxPoint.x, leftMinPoint.y),
			  ImVec2(leftMaxPoint.x, leftMaxPoint.y),
			  ImVec2(rightMinPoint.x, rightMaxPoint.y),
			  ImVec2(rightMinPoint.x, rightMinPoint.y)
			};
			drawList->AddConvexPolyFilled(points.data(), int(points.size()), col);
		}
		struct FrameData
		{
			/*void BuildPriorityTasks(size_t maxPriorityTasksCount)
			{
			  priorityTaskIndices.clear();
			  std::set<std::string> usedTaskNames;

			  for (size_t priorityIndex = 0; priorityIndex < maxPriorityTasksCount; priorityIndex++)
			  {
				size_t bestTaskIndex = size_t(-1);
				for (size_t taskIndex = 0; taskIndex < tasks.size(); taskIndex++)
				{
				  auto &task = tasks[taskIndex];
				  auto it = usedTaskNames.find(tasks[taskIndex].name);
				  if (it == usedTaskNames.end() && (bestTaskIndex == size_t(-1) || tasks[bestTaskIndex].GetLength() < task.GetLength()))
				  {
					bestTaskIndex = taskIndex;
				  }
				}
				if (bestTaskIndex == size_t(-1))
				  break;
				priorityTaskIndices.push_back(bestTaskIndex);
				usedTaskNames.insert(tasks[bestTaskIndex].name);
			  }
			}*/
			std::vector<legit::ProfilerTask> tasks;
			std::vector<size_t> taskStatsIndex;
			//std::vector<size_t> priorityTaskIndices;
		};

		struct TaskStats
		{
			double maxTime;
			size_t priorityOrder;
			size_t onScreenIndex;
		};
		std::vector<TaskStats> taskStats;
		std::map<std::string, size_t> taskNameToStatsIndex;

		/*struct PriorityTask
		{
		  size_t frameIndex;
		  size_t taskIndex;
		};
		std::vector<PriorityTask> priorityTasks;*/
		std::vector<FrameData> frames;
		size_t currFrameIndex = 0;
	};

	class DamageGraph : public ProfilerGraph
	{
	  public:
		std::vector<float> *damage = nullptr;
		std::vector<int> *stage = nullptr;

		DamageGraph(size_t framesCount) : ProfilerGraph(framesCount) {}

		void RenderTimings(int graphWidth, int legendWidth, int height, int frameIndexOffset)
		{
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			const glm::vec2 widgetPos = Vec2(ImGui::GetCursorScreenPos());
			RenderGraph(drawList, widgetPos, glm::vec2(graphWidth, height), frameIndexOffset);
			RenderLegend(drawList, widgetPos + glm::vec2(graphWidth, 0.0f), glm::vec2(legendWidth, height), frameIndexOffset);
			ImGui::Dummy(ImVec2(float(graphWidth + legendWidth), float(height)));
		}

		void RenderGraph(ImDrawList* drawList, glm::vec2 graphPos, glm::vec2 graphSize, size_t frameIndexOffset)
		{
			Rect(drawList, graphPos, graphPos + graphSize, 0xffffffff, false);

			for (size_t x = 0; x < 300 - frameIndexOffset; x++) {
				glm::vec2 framePos = graphPos + glm::vec2(graphSize.x - 1 - frameWidth - (frameWidth + frameSpacing) * x, graphSize.y - 1);
				glm::vec2 taskPos = framePos + glm::vec2(0.0f, 0.0f);
				Rect(drawList,
					taskPos + glm::vec2(0.0f, 0),
					taskPos + glm::vec2(frameWidth, -(float)(*damage)[x + frameIndexOffset] / 1000 * graphSize.y),
					(*stage)[x + frameIndexOffset] == -1 ? legit::Colors::emerald : legit::Colors::carrot,
					true);
			}
		}

		void RenderLegend(ImDrawList* drawList, glm::vec2 legendPos, glm::vec2 legendSize, size_t frameIndexOffset)
		{
			float rectSize = 20;
			glm::vec2 channelPos = legendPos + glm::vec2(legendSize.x / 3, legendSize.y / 2);
			glm::vec2 releasePos = legendPos + glm::vec2(legendSize.x / 3, legendSize.y / 2);
			glm::vec2 rectOffset = glm::vec2(rectSize / 2, rectSize / 2);
			Rect(drawList, channelPos - rectOffset, channelPos + rectOffset, legit::Colors::carrot, true);
			Text(drawList, channelPos + glm::vec2(rectSize, -rectSize / 3), legit::Colors::imguiText, "Channel");

			glm::vec2 nextLegendOffset(0, rectSize);
			Rect(drawList, releasePos - rectOffset + nextLegendOffset, releasePos + rectOffset + nextLegendOffset, legit::Colors::emerald, true);
			Text(drawList, releasePos + glm::vec2(rectSize, -rectSize / 3) + nextLegendOffset, legit::Colors::imguiText, "Release");

			std::ostringstream text;
			text.precision(1);
			text << std::fixed << std::string("Lastest Damage: ") << (damage->back()) << std::string("%");
			Text(drawList, legendPos, legit::Colors::imguiText, text.str().c_str());

			float acc_dmg = 0;
			std::for_each(damage->begin(), damage->end(), [&acc_dmg](float dmg) { acc_dmg += dmg; });

			text.str("");
			text << std::fixed << std::string("Average Damage: ") << (acc_dmg / damage->size()) << std::string("%");
			Text(drawList, legendPos + nextLegendOffset, legit::Colors::imguiText, text.str().c_str());
		}
	};


	class ManaGraph : public ProfilerGraph
	{
	public:
		std::vector<float>* mana = nullptr;
		std::vector<int>* stage = nullptr;

		ManaGraph(size_t framesCount) : ProfilerGraph(framesCount) {}

		void RenderTimings(int graphWidth, int legendWidth, int height, int frameIndexOffset)
		{
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			const glm::vec2 widgetPos = Vec2(ImGui::GetCursorScreenPos());
			RenderGraph(drawList, widgetPos, glm::vec2(graphWidth, height), frameIndexOffset);
			RenderLegend(drawList, widgetPos + glm::vec2(graphWidth, 0.0f), glm::vec2(legendWidth, height), frameIndexOffset);
			ImGui::Dummy(ImVec2(float(graphWidth + legendWidth), float(height)));
		}

		void RenderGraph(ImDrawList* drawList, glm::vec2 graphPos, glm::vec2 graphSize, size_t frameIndexOffset)
		{
			Rect(drawList, graphPos, graphPos + graphSize, 0xffffffff, false);

			for (size_t x = 0; x < 300 - frameIndexOffset; x++) {
				glm::vec2 framePos = graphPos + glm::vec2(graphSize.x - 1 - frameWidth - (frameWidth + frameSpacing) * x, graphSize.y - 1);
				glm::vec2 taskPos = framePos + glm::vec2(0.0f, 0.0f);
				Rect(drawList,
					taskPos + glm::vec2(0.0f, 0),
					taskPos + glm::vec2(frameWidth, -(float)(*mana)[x + frameIndexOffset] / 50 * graphSize.y),
					(*stage)[x + frameIndexOffset] == -1 ? legit::Colors::alizarin : legit::Colors::greenSea,
					true);
			}
		}

		void RenderLegend(ImDrawList* drawList, glm::vec2 legendPos, glm::vec2 legendSize, size_t frameIndexOffset)
		{
			float rectSize = 20;
			glm::vec2 channelPos = legendPos + glm::vec2(legendSize.x / 3, legendSize.y / 2);
			glm::vec2 releasePos = legendPos + glm::vec2(legendSize.x / 3, legendSize.y / 2);
			glm::vec2 rectOffset = glm::vec2(rectSize / 2, rectSize / 2);
			Rect(drawList, channelPos - rectOffset, channelPos + rectOffset, legit::Colors::greenSea, true);
			Text(drawList, channelPos + glm::vec2(rectSize, -rectSize / 3), legit::Colors::imguiText, "Mana");

			glm::vec2 nextLegendOffset(0, rectSize);
			Rect(drawList, releasePos - rectOffset + nextLegendOffset, releasePos + rectOffset + nextLegendOffset, legit::Colors::alizarin, true);
			Text(drawList, releasePos + glm::vec2(rectSize, -rectSize / 3) + nextLegendOffset, legit::Colors::imguiText, "Mana (depleted)");

			std::ostringstream text;
			text.precision(1);
			text << std::fixed << std::string("Current Mana: ") << (mana->back());
			Text(drawList, legendPos, legit::Colors::imguiText, text.str().c_str());
		}
	};

	class ProfilersWindow
	{
	  public:
		ProfilersWindow() :
			dg(300),
			mg(300)
		{
			frameOffset = 0;
			frameWidth = 4;
			frameSpacing = 1;
			useColoredLegendText = true;
		}

		void Render()
		{
			ImGui::SetNextWindowPos(ImVec2(0, 0));
			ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
			ImGui::Begin("Channelling Tick Simulator", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground |
				ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoBringToFrontOnFocus);
			ImVec2 canvasSize = ImGui::GetContentRegionAvail();

			int sizeMargin = int(ImGui::GetStyle().ItemSpacing.y);
			int maxGraphHeight = 300;
			int availableGraphHeight = (int(canvasSize.y) - sizeMargin) / 2;
			int graphHeight = std::min(maxGraphHeight, availableGraphHeight);
			int legendWidth = 200;
			int graphWidth = int(canvasSize.x) - legendWidth;
			//std::cout << graphWidth / (frameWidth + frameSpacing) << std::endl;
			frameOffset = std::max(0, 300 - graphWidth / (frameWidth + frameSpacing));
			dg.RenderTimings(graphWidth, legendWidth, graphHeight, frameOffset);
			mg.RenderTimings(graphWidth, legendWidth, graphHeight, frameOffset);

			dg.frameWidth = frameWidth;
			dg.frameSpacing = frameSpacing;
			dg.useColoredLegendText = useColoredLegendText;
			mg.frameWidth = frameWidth;
			mg.frameSpacing = frameSpacing;
			mg.useColoredLegendText = useColoredLegendText;

			ImGui::End();
		}
		int frameOffset;
		DamageGraph dg;
		ManaGraph mg;
		int frameWidth;
		int frameSpacing;
		bool useColoredLegendText;
	};
}

