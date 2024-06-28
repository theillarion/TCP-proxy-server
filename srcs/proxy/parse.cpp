#include "proxy/parse.hpp"

#include <netinet/in.h>

std::tuple<bool, std::uint64_t, std::string_view> xk7::parse(std::string_view raw_data)
{
	// Структура пакета:
	//	1 байт равный Q (означает, что frontend отправил backend'у запрос)
	//	4-ех байтовое целое неотрицательное число отвечает за количество байт в сообщении + 4 (данное число также включено)
	//  String в СИ-стиле, т.е. с кареткой завершения строки '\0' 

	// raw_data должно содержать как минимум 5 байтов (первый байт идентифицирует тип пакета, оставшиейся 4 - размер данных)
	// https://www.postgresql.org/docs/16/protocol-message-formats.html#PROTOCOL-MESSAGE-FORMATS-QUERY
	if (raw_data.size() < 5 || raw_data[0] != 'Q')
		return { false, 0, {} };

	// работает как с байтами 
	auto bytes = reinterpret_cast<const unsigned char*>(raw_data.data()) + 1;

	// преобразование из network byte order в host byte order
	auto count_bytes = ntohl(*reinterpret_cast<const std::uint32_t*>(bytes));

	// в общее количество байт включено количество байт данного числа
	if (count_bytes < 4)
		return { false, count_bytes, {} };
	count_bytes -= 4;

	return { true, count_bytes, std::string_view(reinterpret_cast<const char*>(bytes + 4)) };
}
