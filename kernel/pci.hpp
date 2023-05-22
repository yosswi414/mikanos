/**
 * @file pci.hpp
 * @brief PCI バス制御のプログラムを集めたファイル．
 */

#pragma once

#include <array>
#include <cstdint>

#include "error.hpp"

namespace pci {
    // #@@range_begin(config_addr)
    /** @brief CONFIG_ADDRESS レジスタの IO ポートアドレス */
    const uint16_t kConfigAddress = 0x0cf8;
    /** @brief CONFIG_DATA レジスタの IO ポートアドレス */
    const uint16_t kConfigData = 0x0cfc;
    // #@@range_end(config_addr)

    // #@@range_begin(class_code)
    /** @brief PCI デバイスのクラスコード */
    struct ClassCode {
        uint8_t base, sub, interface;

        /** @brief ベースクラスが等しい場合に真 */
        bool Match(uint8_t b) const { return b == base; }
        /** @brief サブクラスまで等しい場合に真 */
        bool Match(uint8_t b, uint8_t s) const { return Match(b) && s == sub; }
        /** @brief インターフェースまで等しい場合に真 */
        bool Match(uint8_t b, uint8_t s, uint8_t i) const { return Match(b, s) && i == interface; }
    };

    /** @brief PCI デバイスを操作するための基礎データを格納する
     * バス番号，デバイス番号，ファンクション番号はデバイスを特定するのに必須．
     * その他の情報は単に利便性のために加えてある．
     * */
    struct Device {
        uint8_t bus, device, function, header_type;
        ClassCode class_code;
    };
    // #@@range_end(class_code)

    /** @brief CONFIG_ADDRESS に指定された整数を書き込む */
    void WriteAddress(uint32_t address);
    /** @brief CONFIG_DATA に指定された整数を書き込む */
    void WriteData(uint32_t value);
    /** @brief CONFIG_DATA から 32 ビット整数を読み込む */
    uint32_t ReadData();

    /** @brief ベンダ ID レジスタを読み取る（全ヘッダタイプ共通） */
    uint16_t ReadVendorId(uint8_t bus, uint8_t device, uint8_t function);
    /** @brief デバイス ID レジスタを読み取る（全ヘッダタイプ共通） */
    uint16_t ReadDeviceId(uint8_t bus, uint8_t device, uint8_t function);
    /** @brief ヘッダタイプレジスタを読み取る（全ヘッダタイプ共通） */
    uint8_t ReadHeaderType(uint8_t bus, uint8_t device, uint8_t function);
    /** @brief クラスコードレジスタを読み取る（全ヘッダタイプ共通）
     * 返される 32 ビット整数の構造は次の通り．
     *   - 31:24 : ベースクラス (Base Class)
     *   - 23:16 : サブクラス (Sub Class)
     *   - 15:8  : インターフェース (Interface)
     *   - 7:0   : リビジョン (Revision ID)
     */
    ClassCode ReadClassCode(uint8_t bus, uint8_t device, uint8_t function);

    inline uint16_t ReadVendorId(const Device& dev) {
        return ReadVendorId(dev.bus, dev.device, dev.function);
    }

    /** @brief 指定された PCI デバイスの 32 ビットレジスタを読み取る */
    uint32_t ReadConfReg(const Device& dev, uint8_t reg_addr);

    void WriteConfReg(const Device& dev, uint8_t reg_addr, uint32_t value);

    /** @brief バス番号レジスタを読み取る（ヘッダタイプ 1 用）
     * 返される 32 ビット整数の構造は次の通り．
     *   - 23:16 : サブオーディネイトバス番号
     *   - 15:8  : セカンダリバス番号
     *   - 7:0   : リビジョン番号
     */
    uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t function);

    /** @brief 単一ファンクションの場合に真を返す． */
    bool IsSingleFunctionDevice(uint8_t header_type);

    // #@@range_begin(var_devices)
    /** @brief ScanAllBus() により発見された PCI デバイスの一覧 */
    inline std::array<Device, 32> devices;
    /** @brief devices の有効な要素の数 */
    inline int num_device;
    /** @brief PCI デバイスをすべて探索し devices に格納する
     *
     * バス 0 から再帰的に PCI デバイスを探索し，devices の先頭から詰めて書き込む．
     * 発見したデバイスの数を num_devices に設定する．
     */
    Error ScanAllBus();
    // #@@range_end(var_devices)

    constexpr uint8_t CalcBarAddress(unsigned int bar_index) { return 0x10 + 4 * bar_index; }

    /**
     * @brief BAR (Base Address Register) の読み取り
     * PCI コンフィグレーション空間にある BARx を読み取る
     * @param device PCI デバイス
     * @param bar_index BARx (x = bar_index) を返す
     * @return WithError<uint64_t> 指定された BAR と後続の BAR を読み、結合したアドレス (uint64_t) を返す
     */
    WithError<uint64_t> ReadBar(Device& device, unsigned int bar_index);

    /** @brief PCI capability レジスタの共通ヘッダ */
    union CapabilityHeader {
        uint32_t data;
        struct {
            uint32_t cap_id : 8;
            uint32_t next_ptr : 8;
            uint32_t cap : 16;
        } __attribute__((packed)) bits;
    } __attribute__((packed));

    const uint8_t kCapabilityMSI = 0x05;
    const uint8_t kCapabilityMSIX = 0x11;

    /**
     * @brief 指定された PCI デバイスの指定された capability レジスタを読み込む
     *
     * @param dev capability を読み込む PCI デバイス
     * @param addr capability レジスタの configuration 空間アドレス
     */
    CapabilityHeader ReadCapabilityHeader(const Device& dev, uint8_t addr);

    /**
     * @brief MSI capability 構造
     *
     * MSI capability 構造は 64 bit サポートの有無などで亜種が沢山ある
     * この構造体は各亜種に対応するために最大の亜種に合わせてメンバを定義してある
     */
    struct MSICapability {
        union {
            uint32_t data;
            struct {
                uint32_t cap_id : 8;
                uint32_t next_ptr : 8;
                uint32_t msi_enable : 1;
                uint32_t multi_msg_capable : 3;
                uint32_t multi_msg_enable : 3;
                uint32_t addr_64_capable : 1;
                uint32_t per_vector_mask_capable : 1;
                uint32_t : 7;
            } __attribute__((packed)) bits;
        } __attribute__((packed)) header;

        uint32_t msg_addr;
        uint32_t msg_upper_addr;
        uint32_t msg_data;
        uint32_t mask_bits;
        uint32_t pending_bits;
    } __attribute__((packed));

    /**
     * @brief MSI または MSI-X 割り込み設定
     *
     * @param dev 設定対象の PCI デバイス
     * @param msg_addr 割り込み発生時にメッセージを書き込む先のアドレス
     * @param msg_data 割り込み発生時に書き込むメッセージの値
     * @param num_vector_exponent 割り当てるベクタ数 (2 冪の指数で指定)
     */
    Error ConfigureMSI(const Device& dev, uint32_t msg_addr, uint32_t msg_data,
                       unsigned int num_vector_exponent);

    enum class MSITriggerMode : uint8_t {
        kEdge = 0,
        kLevel = 1
    };

    enum class MSIDeliveryMode : uint8_t {
        kFixed = 0b000,
        kLowestPriority = 0b001,
        kSMI = 0b010,
        kNMI = 0b100,
        kINIT = 0b101,
        kExtINT = 0b111,
    };

    union MessageAddress {
        MessageAddress():bits{0,0,0,0,0xfee}{}
        uint32_t data;
        struct {
            uint8_t xx : 2;
            uint8_t destination_mode : 1;
            // (p.169) mikanOS では 0
            uint8_t redirection_hint : 1;
            uint8_t : 8;
            uint8_t destination_id : 8;
            // 0xfee で固定
            uint16_t addr_tail : 12;
        } __attribute__((packed)) bits;
    } __attribute__((packed));

    union MessageData {
        uint32_t data;
        struct {
            uint8_t vector : 8;
            MSIDeliveryMode delivery_mode : 3;
            uint8_t : 3;
            uint8_t level : 1;
            MSITriggerMode trigger_mode : 1;
            uint16_t : 16;
        } __attribute__((packed)) bits;
    } __attribute__((packed));

    /**
     * @brief MSI 割り込みを有効化するための設定を行う
     *
     * @param dev 設定対象の PCI デバイス
     * @param apic_id Destination ID フィールド (Message Address)
     * @param trigger_mode
     * @param delivery_mode
     * @param vector Vector フィールド (Message Data)
     * @param num_vector_exponent
     * @return Error
     */
    Error ConfigureMSIFixedDestination(
        const Device& dev, uint8_t apic_id,
        MSITriggerMode trigger_mode, MSIDeliveryMode delivery_mode,
        uint8_t vector, unsigned int num_vector_exponent);
}  // namespace pci
