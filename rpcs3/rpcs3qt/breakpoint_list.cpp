#include "breakpoint_list.h"
#include "breakpoint_handler.h"

#include "Emu/CPU/CPUDisAsm.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/PPUThread.h"

#include <QMenu>

constexpr auto qstr = QString::fromStdString;

breakpoint_list::breakpoint_list(QWidget* parent) : QListWidget(parent)
{
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setContextMenuPolicy(Qt::CustomContextMenu);
	setSelectionMode(QAbstractItemView::ExtendedSelection);

	// connects
	connect(this, &QListWidget::itemDoubleClicked, this, &breakpoint_list::OnBreakpointListDoubleClicked);
	connect(this, &QListWidget::customContextMenuRequested, this, &breakpoint_list::OnBreakpointListRightClicked);
}

/**
* It's unfortunate I need a method like this to sync these.  Should ponder a cleaner way to do this.
*/
void breakpoint_list::UpdateCPUData(cpu_thread* cpu, CPUDisAsm* disasm)
{
	m_cpu = cpu;
	m_disasm = disasm;
}

void breakpoint_list::ClearBreakpoints()
{
	const auto cpu = this->cpu.lock();

	while (count())
	{
		auto* currentItem = takeItem(0);
		u32 loc = currentItem->data(Qt::UserRole).value<u32>();

		if (cpu)
		{
			g_fxo->init<breakpoint_handler>()->remove(loc, cpu->id);
		}

		delete currentItem;
	}
}

void breakpoint_list::RemoveBreakpoint(u32 addr)
{
	const auto cpu = this->cpu.lock();

	if (cpu)
	{
		g_fxo->init<breakpoint_handler>()->remove(addr, cpu->id);
	}

	for (int i = 0; i < count(); i++)
	{
		QListWidgetItem* currentItem = item(i);

		if (currentItem->data(Qt::UserRole).value<u32>() == addr)
		{
			delete takeItem(i);
			break;
		}
	}

	Q_EMIT RequestShowAddress(addr);
}

void breakpoint_list::AddBreakpoint(u32 pc)
{
	g_fxo->init<breakpoint_handler>()->add(pc, bp_type::read + bp_type::write + bp_type::debug, breakpoint_handler::all_threads(m_cpu->id_type()));

	m_disasm->disasm(pc);

	QString breakpointItemText = qstr(m_disasm->last_opcode);

	breakpointItemText.remove(10, 13);

	QListWidgetItem* breakpointItem = new QListWidgetItem(breakpointItemText);
	breakpointItem->setForeground(m_text_color_bp);
	breakpointItem->setBackground(m_color_bp);
	QVariant pcVariant;
	pcVariant.setValue(pc);
	breakpointItem->setData(Qt::UserRole, pcVariant);
	addItem(breakpointItem);

	Q_EMIT RequestShowAddress(pc);
}

/**
* If breakpoint exists, we remove it, else add new one.  Yeah, it'd be nicer from a code logic to have it be set/reset.  But, that logic has to happen somewhere anyhow.
*/
void breakpoint_list::HandleBreakpointRequest(u32 loc)
{
	if (!m_cpu || !m_cpu->id_type())
	{
		return;
	}

	const auto handler = g_fxo->init<breakpoint_handler>();
	using bp_type = enum breakpoint_handler::bp_type;

	handler->toggle(loc, cpu->id, bp_type::read + bp_type::write + bp_type::debug);
}

void breakpoint_list::OnBreakpointListDoubleClicked()
{
	u32 address = currentItem()->data(Qt::UserRole).value<u32>();
	Q_EMIT RequestShowAddress(address);
}

void breakpoint_list::OnBreakpointListRightClicked(const QPoint &pos)
{
	if (!itemAt(pos))
	{
		return;
	}

	QMenu* menu = new QMenu();

	if (selectedItems().count() == 1)
	{
		menu->addAction("Rename");
		menu->addSeparator();
	}

	QAction* m_breakpoint_list_delete = new QAction("Delete", this);
	m_breakpoint_list_delete->setShortcut(Qt::Key_Delete);
	m_breakpoint_list_delete->setShortcutContext(Qt::WidgetShortcut);
	addAction(m_breakpoint_list_delete);
	connect(m_breakpoint_list_delete, &QAction::triggered, this, &breakpoint_list::OnBreakpointListDelete);

	menu->addAction(m_breakpoint_list_delete);

	QAction* selectedItem = menu->exec(viewport()->mapToGlobal(pos));
	if (selectedItem)
	{
		if (selectedItem->text() == "Rename")
		{
			QListWidgetItem* currentItem = selectedItems().at(0);

			currentItem->setFlags(currentItem->flags() | Qt::ItemIsEditable);
			editItem(currentItem);
		}
	}
}

void breakpoint_list::OnBreakpointListDelete()
{
	int selectedCount = selectedItems().count();

	for (int i = selectedCount - 1; i >= 0; i--)
	{
		RemoveBreakpoint(item(i)->data(Qt::UserRole).value<u32>());
	}
}
