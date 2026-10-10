#include "EnginePCH.h"
#include "Editor/OutputLog/OutputLogPanel.h"

#include "Core/StatOverlay.h"

static int TextEditCallbackStub(ImGuiInputTextCallbackData* data)
{
	FOutputLogPanel* OutputLog = (FOutputLogPanel*)data->UserData;
	return OutputLog->TextEditCallback(data);
}

static int   Strnicmp(const char* s1, const char* s2, int n) { int d = 0; while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; n--; } return d; }

// 콘솔 명령은 입력 대소문자를 가리지 않는다. "stat fps"와 "STAT FPS"가 같아야 한다.
static bool EqualsIgnoreCase(const FString& A, const char* B)
{
	return Strnicmp(A.c_str(), B, static_cast<int>(A.size()) + 1) == 0;
}

bool FOutputLogPanel::Init()
{
	ClearLog();

	Commands.Add("HELP");
	Commands.Add("HISTORY");
	Commands.Add("CLEAR");
	Commands.Add("STAT");

	HistoryPos = -1;
	InputBuf[0] = '\0';

	AutoScroll = true;
	ScrollToBottom = false;

	return true;
}

void FOutputLogPanel::Tick(float DeltaTime)
{
}

void FOutputLogPanel::OnRender()
{
	ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);

	ImGui::Begin("Output Log");

	//////////////////////////////////////////////////////////

	ImGui::TextDisabled("Filter");
	ImGui::SameLine();

	Filter.Draw("##Filter", 180.0f);

	ImGui::SameLine();
	ImGui::TextDisabled("[?]");

	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip(
			"Filter syntax:\n"
			"error        Include matching lines\n"
			"-warning     Exclude matching lines\n"
			"error,-warn  Combine filters"
		);
	}

	ImGui::SameLine();
	ImGui::Checkbox("Auto-scroll", &AutoScroll);

	ImGui::SameLine();
	if (ImGui::SmallButton("Clear"))
		ClearLog();

	ImGui::SameLine();
	bool CopyToClipboard = ImGui::SmallButton("Copy");

	ImGui::Separator();

	//////////////////////////////////////////////////////////

	ImGuiStyle& style = ImGui::GetStyle();
	const float footer_height_to_reserve = style.SeparatorSize + style.ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
	if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), ImGuiChildFlags_NavFlattened, ImGuiWindowFlags_HorizontalScrollbar)) // ImGui Window 안에 Child Window를 생성
	{
		if (ImGui::BeginPopupContextWindow())
		{
			if (ImGui::Selectable("Clear"))
				ClearLog();
			ImGui::EndPopup();
		}

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
		if (CopyToClipboard)
			ImGui::LogToClipboard();
		for (const FLogData& item : Items)
		{
			if (!Filter.PassFilter(item.message.c_str()))
				continue;

			ImVec4 color = item.Color;
			ImGui::PushStyleColor(ImGuiCol_Text, color);
			ImGui::TextUnformatted(item.message.c_str());
			ImGui::PopStyleColor();
		}
		if (CopyToClipboard)
			ImGui::LogFinish();
		if (ScrollToBottom || (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
			ImGui::SetScrollHereY(1.0f);
		ScrollToBottom = false;

		ImGui::PopStyleVar();
	}
	ImGui::EndChild();
	ImGui::Separator();

	//////////////////////////////////////////////////////////

	bool reclaim_focus = false;
	ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
	ImGui::TextDisabled(">");

	ImGui::SameLine();

	ImGui::SetNextItemWidth(-1.0f);

	if (ImGui::InputTextWithHint("##OutputLogInput", "Enter command...  (HELP for commands)", InputBuf, IM_COUNTOF(InputBuf), input_text_flags, &TextEditCallbackStub, this))
	{
		char* s = InputBuf;
		if (s[0])
		{
			ExecCommand(s);
		}
		s[0] = 0;
		reclaim_focus = true;
	}

	ImGui::SetItemDefaultFocus();
	if (reclaim_focus)
		ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget

	//////////////////////////////////////////////////////////

	ImGui::End();
}

void FOutputLogPanel::ClearLog()
{
	Items.Reset();
}

void FOutputLogPanel::ExecCommand(const FString& CommandLine)
{
	FLog::Log(ELogVerbosity::Input, "# {}\n", CommandLine);

	// Insert into history. First find match and delete it so it can be pushed to the back.
	// This isn't trying to be smart or optimal.
	HistoryPos = -1;
	for (int i = History.Num() - 1; i >= 0; i--)
		if (History[i] == CommandLine)
		{
			//History.erase(History.begin() + i);
			History.RemoveAt(i, 1);
			break;
		}
	History.Add(CommandLine);

	// Process command
	// 인자를 받는 명령은 문자열 전체 비교로는 걸러지지 않으므로 먼저 위임한다.
	FString StatMessage;
	if (FStatOverlay::ExecCommand(CommandLine, StatMessage))
	{
		FLog::Log(ELogVerbosity::Info, "{}", StatMessage);
		ScrollToBottom = true;
		return;
	}

	if (EqualsIgnoreCase(CommandLine, "CLEAR"))
	{
		ClearLog();
	}
	else if (EqualsIgnoreCase(CommandLine, "HELP"))
	{
		FLog::Log(ELogVerbosity::Info, "Commands:");
		for (const FString& Command : Commands)
			FLog::Log(ELogVerbosity::Info, "- {}", Command);
		OnLog(ELogVerbosity::Info, "- STAT <fps|memory|all|none>");
	}
	else if (EqualsIgnoreCase(CommandLine, "HISTORY"))
	{
		int first = History.Num() - 10;
		for (int i = first > 0 ? first : 0; i < History.Num(); i++)
			FLog::Log(ELogVerbosity::Info, "{:03d}: {}\n", i, History[i]);
	}
	else
	{
		FLog::Log(ELogVerbosity::Info, "Unknown command: '{}'\n", CommandLine);
	}

	// On command input, we scroll to bottom even if AutoScroll==false
	ScrollToBottom = true;
}

int FOutputLogPanel::TextEditCallback(ImGuiInputTextCallbackData* data)
{
	//AddLog("cursor: %d, selection: %d-%d", data->CursorPos, data->SelectionStart, data->SelectionEnd);
	switch (data->EventFlag)
	{
	case ImGuiInputTextFlags_CallbackCompletion:
	{
		// Example of TEXT COMPLETION

		// Locate beginning of current word
		const char* word_end = data->Buf + data->CursorPos;
		const char* word_start = word_end;
		while (word_start > data->Buf)
		{
			const char c = word_start[-1];
			if (c == ' ' || c == '\t' || c == ',' || c == ';')
				break;
			word_start--;
		}

		// Build a list of candidates
		ImVector<FString> candidates;
		for (int i = 0; i < Commands.Num(); i++)
			if (Strnicmp(Commands[i].c_str(), word_start, (int)(word_end - word_start)) == 0)
				candidates.push_back(Commands[i]);

		if (candidates.Size == 0)
		{
			// No match
			FLog::Log(ELogVerbosity::Input, "No match for \"{}\"!\n", word_start);
		}
		else if (candidates.Size == 1)
		{
			// Single match. Delete the beginning of the word and replace it entirely so we've got nice casing.
			data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
			data->InsertChars(data->CursorPos, candidates[0].c_str());
			data->InsertChars(data->CursorPos, " ");
		}
		else
		{
			// Multiple matches. Complete as much as we can..
			// So inputting "C"+Tab will complete to "CL" then display "CLEAR" and "CLASSIFY" as matches.
			int match_len = (int)(word_end - word_start);
			for (;;)
			{
				int c = 0;
				bool all_candidates_matches = true;
				for (int i = 0; i < candidates.Size && all_candidates_matches; i++)
					if (i == 0)
						c = toupper(candidates[i][match_len]);
					else if (c == 0 || c != toupper(candidates[i][match_len]))
						all_candidates_matches = false;
				if (!all_candidates_matches)
					break;
				match_len++;
			}

			if (match_len > 0)
			{
				data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
				data->InsertChars(data->CursorPos, candidates[0].c_str(), candidates[0].c_str() + match_len);
			}

			// List matches
			FLog::Log(ELogVerbosity::Info, "Possible matches:\n");
			for (int i = 0; i < candidates.Size; i++)
				FLog::Log(ELogVerbosity::Info, "- {}\n", candidates[i]);
		}

		break;
	}
	case ImGuiInputTextFlags_CallbackHistory:
	{
		// Example of HISTORY
		const int prev_history_pos = HistoryPos;
		if (data->EventKey == ImGuiKey_UpArrow)
		{
			if (HistoryPos == -1)
				HistoryPos = History.Num() - 1;
			else if (HistoryPos > 0)
				HistoryPos--;
		}
		else if (data->EventKey == ImGuiKey_DownArrow)
		{
			if (HistoryPos != -1)
				if (++HistoryPos >= History.Num())
					HistoryPos = -1;
		}

		// A better implementation would preserve the data on the current input line along with cursor position.
		if (prev_history_pos != HistoryPos)
		{
			const char* history_str = (HistoryPos >= 0) ? History[HistoryPos].c_str() : "";
			data->DeleteChars(0, data->BufTextLen);
			data->InsertChars(0, history_str);
		}
	}
	}
	return 0;
}

void FOutputLogPanel::OnLog(ELogVerbosity Verbosity, const FString& Message)
{
	FLogData LogData;

	switch (Verbosity)
	{
	case ELogVerbosity::Input:
		LogData.Color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f);
		break;
	case ELogVerbosity::Info:
		LogData.Color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		break;
	case ELogVerbosity::Warning:
		LogData.Color = ImVec4(1.0f, 0.75f, 0.2f, 1.0f);
		break;
	case ELogVerbosity::Error:
		LogData.Color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
		break;
	}
	LogData.message = Message;
	Items.Add(LogData);
	ScrollToBottom = true;
}
